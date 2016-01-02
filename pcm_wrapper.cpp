#include <iostream>
#include <string>

#ifdef __linux__
  #include <alsa/asoundlib.h>
  #include <alsa/pcm.h>
  static std::string device = "default";
  static snd_output_t *output = 0;
  static snd_pcm_t *handle;
  static int err;
#else
  #include <fstream>
  #include <thread>
  #include <chrono>
  using namespace std;
  static ofstream pcm_out("/opt/chippy_files/pcm_out.txt");
#endif

using namespace std;
#include "pcm_wrapper.h"
#include "util.h"


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
#else

    /*
    for (int i = 0; i < BUFFER_LEN; ++i) {
        for (int j = 0; j < audio_buffer[i] / 2; ++j) {
            pcm_out << ' ';
        }
        pcm_out << 'o' << endl;
    }

    this_thread::sleep_for(chrono::milliseconds(100));
    */

#endif
}

void pcm_close() {
#ifdef __linux__
    snd_pcm_close(handle);
#else
    pcm_out.close();
#endif
}

struct wav_file_header {
    wav_file_header(uint32_t data_size):
        size(sizeof(wav_file_header) + data_size * 2 - 8),
        data_size(data_size * 2) {
    }
        
    const uint32_t riff               {0x52494646};
    const uint32_t size;
    const uint32_t wave               {0x57415645};
    const uint32_t fmt_label          {0x666d7420};
    const uint32_t fmt_length         {16};
    const uint16_t fmt_type           {1};
    const uint16_t fmt_num_channels   {1};
    const uint32_t fmt_sample_rate    {SAMPLE_FREQUENCY};
    const uint32_t fmt_bps            {SAMPLE_FREQUENCY * 2};
    const uint16_t fmt_block_align    {16};
    const uint16_t fmt_bitrate        {16};
    const uint32_t data_label         {0x64616461};
    const uint32_t data_size;
};

#include <cmath>
void pcm_write_wav(ostream &out, const uint8_t *data, const uint32_t data_size) {
    wav_file_header header(data_size);
    write_stream(out,    to_big_endian(header.riff));
    write_stream(out, to_little_endian(header.size));
    write_stream(out,    to_big_endian(header.wave));
    write_stream(out,    to_big_endian(header.fmt_label));
    write_stream(out, to_little_endian(header.fmt_length));
    write_stream(out, to_little_endian(header.fmt_type));
    write_stream(out, to_little_endian(header.fmt_num_channels));
    write_stream(out, to_little_endian(header.fmt_sample_rate));
    write_stream(out, to_little_endian(header.fmt_bps));
    write_stream(out, to_little_endian(header.fmt_block_align));
    write_stream(out, to_little_endian(header.fmt_bitrate));
    write_stream(out,    to_big_endian(header.data_label));
    write_stream(out, to_little_endian(header.data_size));
    for (uint32_t i = 0; i < data_size; ++i) {
        int16_t value = sin(((double) i * 2 * 3.14159 * 440 ) / SAMPLE_FREQUENCY) * 0xffff;
        out.put(value & 0xff).put((value << 8) & 0xff);
    }
    //out.write(reinterpret_cast<const char*>(data), data_size);
}


#ifdef WAV_TEST
#include <iostream>
int main(void) {
    const uint32_t size = SAMPLE_FREQUENCY * 3;
    uint8_t wav_data[size];
    for (int i = 0; i < size; ++i) {
        wav_data[i] = (uint8_t) i / 10;
    }

    ofstream out("/opt/chippy_files/test.wav");
    pcm_write_wav(out, wav_data, SAMPLE_FREQUENCY * 3);
    out.close();

    return 0;
}
#endif



