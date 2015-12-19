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
    cout << "xrun_recovery\n";
    if (err == -EPIPE) {    
        cout << "underrun\n";
        // under-run
        snd_pcm_prepare(handle);
        err = 0;
    } else if (err == -ESTRPIPE) {
        cout << "suspended\n";
        // wait until the suspend flag is released 
        while ((err = snd_pcm_resume(handle)) == -EAGAIN)
            sleep(1);       
        if (err < 0) {
            snd_pcm_prepare(handle);
        }
        err = 0;
    }
}

static void wait_for_poll() {
    cout << "get pd count\n";
    int count = snd_pcm_poll_descriptors_count(handle);
    if (count <= 0) {
        throw runtime_error("invalid poll descriptors count");
    }

    cout << "allocate pd's\n";
    static struct pollfd *ufds = (struct pollfd*) malloc(count * sizeof(struct pollfd));
    cout << "get pd's\n";
    if ((err = snd_pcm_poll_descriptors(handle, ufds, count)) < 0) {
        throw runtime_error("unable to obtain poll descriptors for playback: " + string(snd_strerror(err)));
    }
    cout << "got " << count << " pd's\n";
    uint16_t revents;
    while (1) {
        cout << "poll\n";
        poll(ufds, count, -1);
        cout << "revents\n";
        snd_pcm_poll_descriptors_revents(handle, ufds, count, &revents);
        if (revents & POLLERR) {
            cout << "POLLERR\n";
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
    cout << "pcm_open\n";
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
#else
    cout << "pcm_open();\n";
#endif
}



void pcm_write() {

#ifdef __linux__

    wait_for_poll();

    int len = BUFFER_LEN;
    while (len) {
        snd_pcm_sframes_t frames = snd_pcm_writei(handle, audio_buffer, BUFFER_LEN);
        if (frames < 0) {
            frames = snd_pcm_recover(handle, frames, 0);
            if (frames < 0) {
                throw runtime_error("unable to write PCM data: " + string(snd_strerror(frames)));
            }
        }
        len -= frames;
    }
#else
    for (int i = 0; i < BUFFER_LEN; ++i) {
        cout << "+";
        for (int j = 0; j < audio_buffer[i]; j += 2) {
            cout << " ";
        }
        cout << ".\n";
    }
#endif
}

void pcm_close() {
#ifdef __linux__
    snd_pcm_close(handle);
#else
    cout << "pcm_close()\n";
#endif
}

