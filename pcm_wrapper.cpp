#include <iostream>
#include <string>

#ifdef __linux__
  #include <alsa/asoundlib.h>
  #include <alsa/pcm.h>
  static std::string device = "default";
  static snd_output_t *output = 0;
  static snd_pcm_t *handle;
  static int err;
#endif

using namespace std;
#include "pcm_wrapper.h"

uint8_t audio_buffer[BUFFER_LEN];

#ifdef __linux__
static void xrun_recovery() {
    if (err == -EPIPE) {    
        // under-run
        snd_pcm_prepare(handle);
        err = 0;
    } else if (err == -ESTRPIPE) {
        // wait until the suspend flag is released 
        while ((err = snd_pcm_resume(handle)) == -EAGAIN)
            sleep(1);       
        if (err < 0) {
            snd_pcm_prepare(handle);
        }
        err = 0;
    }
}

static void wait_for_poll(struct pollfd *ufds, int fdcount) {
    uint16_t revents;
    while (1) {
        poll(ufds, fdcount, -1);
        snd_pcm_poll_descriptors_revents(handle, ufds, fdcount, &revents);
        if (revents & POLLERR) {
            if (err < 0) {
                if (snd_pcm_state(handle) == SND_PCM_STATE_XRUN ||
                    snd_pcm_state(handle) == SND_PCM_STATE_SUSPENDED) {
                    err = snd_pcm_state(handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                    xrun_recovery();
                    if (err < 0) {
                        throw runtime_error("write error: " + string(snd_strerror(err)));
                    }
                } else {
                    throw runtime_error("wait for poll failed: " + string(snd_strerror(err)));
                }
            }
            xrun_recovery();
            if (err < 0) {
                throw runtime_error("unable to poll: " + string(snd_strerror(err)));
            }
        }
        if (revents & POLLOUT)
            break;
    }
}

#endif

void pcm_open(void) {
#ifdef __linux__
    if ((err = snd_pcm_open(&handle, device.c_str(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        throw runtime_error("unable to open PCM device: " + string(snd_strerror(err)));
    }

    if ((err = snd_pcm_set_params(handle,
                                  SND_PCM_FORMAT_U8,
                                  SND_PCM_ACCESS_RW_INTERLEAVED,
                                  1,
                                  SAMPLE_FREQUENCY,
                                  1,
                                  500000)) < 0) {   
        throw runtime_error("unable to set PCM params: " + string(snd_strerror(err)));
    }
#endif
}


static bool need_poll = false;
void pcm_write() {

#ifdef __linux__

    int fdcount = snd_pcm_poll_descriptors_count(handle);
    if (fdcount <= 0) {
        throw runtime_error("invalid poll descriptors count");
    }

    static struct pollfd *ufds = (struct pollfd*) malloc(fdcount * sizeof(struct pollfd));
    if ((err = snd_pcm_poll_descriptors(handle, ufds, fdcount)) < 0) {
        throw runtime_error("unable to obtain poll descriptors for playback: " + string(snd_strerror(err)));
    }

    int len = BUFFER_LEN;
    uint8_t *buffer = audio_buffer;
    while (1) {
        snd_pcm_sframes_t frames = snd_pcm_writei(handle, buffer, BUFFER_LEN);
        if (frames < 0) {
            frames = snd_pcm_recover(handle, frames, 0);
            if (frames < 0) {
                throw runtime_error("unable to write PCM data: " + string(snd_strerror(frames)));
            }
        }
        if (snd_pcm_state(handle) == SND_PCM_STATE_RUNNING)
            need_poll = true;

        len -= frames;
        buffer += frames;
        if (len <= 0) 
            break;

        wait_for_poll(ufds, fdcount);
        need_poll = false;
    }
#endif
}

void pcm_close() {
#ifdef __linux__
    snd_pcm_close(handle);
#endif
}

