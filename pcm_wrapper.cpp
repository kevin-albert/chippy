#include <iostream>
#include <string>

#ifdef linux
  #include <alsa/asoundlib.h>
  #include <alsa/pcm.h>
  static const string device = "default";
  static snd_output_t *output = 0;
  static snd_pcm_t *handle;
#endif

using namespace std;
#include "pcm_wrapper.h"

uint8_t audio_buffer[BUFFER_LEN];

void pcm_open(void) {
#ifdef linux
    int err;
    if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        throw runtime_error("unable to open PCM device: " + snd_strerror(err));
    }

    if ((err = snd_pcm_set_params(handle,
                                  SND_PCM_FORMAT_U8,
                                  SND_PCM_ACCESS_RW_INTERLEAVED,
                                  1,
                                  SAMPLE_FREQUENCY,
                                  1,
                                  500000)) < 0) {   
        throw runtime_error("unable to set PCM params: " + snd_strerror(err));
    }
#else
    cout << "pcm_open();\n";
#endif
}



void pcm_write() {
#ifdef linux
    snd_pcm_sframes_t frames = snd_pcm_writei(handle, audio_buffer, BUFFER_LEN);
    if (frames < 0) {
        frames = snd_pcm_recover(handle, frames, 0);
        if (frames < 0) {
            throw runtime_error("unable to write PCM data: " + snd_strerror(frames));
        }
    }
    if (frames < BUFFER_LEN) {
        throw runtime_error("unable to write some PCM data");
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
#ifdef linux
    snd_pcm_close(handle);
#else
    cout << "pcm_close()\n";
#endif
}

