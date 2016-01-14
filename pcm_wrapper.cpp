#include <iostream>
#include <string>

using namespace std;

#include "pcm_wrapper.h"
#include "util.h"

#ifdef __linux__
  #include <alsa/asoundlib.h>
  #include <alsa/pcm.h>
  static std::string device = "default";
  static snd_output_t *output = 0;
  static snd_pcm_t *handle;
  static int err;
#else
  #include <mutex>
  #include <condition_variable>
  #include <portaudio.h>
  PaStreamParameters pa_params;
  mutex mtx;
  mutex callback_mtx;
  condition_variable is_done;
  bool playing {false};
  struct pa_data_loader {
      pa_data_loader(bool (get_data(SAMPLE*, uint32_t))):
          get_data(get_data) {}
      bool (*get_data)(SAMPLE*, uint32_t);
  };
  #define pa_call(func) {\
    PaError err = func;\
    if (err != paNoError) {\
        string msg = Pa_GetErrorText(err);\
        throw new runtime_error(msg);\
    }\
  }
#endif


//SAMPLE audio_buffer[BUFFER_LEN];

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

#else


int pa_callback(const void *input_buffer, void *output_buffer, unsigned long frames_per_buffer,
                const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags,
                void *user_data) {
    trace("");
    (void) time_info;
    (void) status_flags;
    (void) input_buffer;
    SAMPLE *out = (SAMPLE*) output_buffer;

    trace(reinterpret_cast<void*>((((pa_data_loader*) user_data)->get_data)));
    if (((pa_data_loader*) user_data)->get_data(out, frames_per_buffer)) {
        trace("return paContinue");
        return paContinue;
    } else {
        trace("return paComplete");
        return paComplete;
    }
}


void pa_finished(void *user_data) {
    trace("");
    (void) user_data;
    playing = false;
    is_done.notify_one();
}

bool pa_done_playing() {
    return !playing;
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
                                  1000000)) < 0) {   
        throw runtime_error("unable to set PCM params: " + string(snd_strerror(err)));
    }
#else
    trace("initializing");
    pa_call(Pa_Initialize());
    pa_params.device = Pa_GetDefaultOutputDevice(); 
    if (pa_params.device == paNoDevice) {
        throw runtime_error("no default output device");
    }
    pa_params.channelCount = 2;
    pa_params.sampleFormat = paFloat32;//paUInt8;
    pa_params.suggestedLatency = Pa_GetDeviceInfo(pa_params.device)->defaultLowOutputLatency;
    pa_params.hostApiSpecificStreamInfo = nullptr;
    trace("done");
#endif
}

#ifndef __linux__
void pcm_play(bool (get_data(SAMPLE*, uint32_t))) {

    trace("opening stream");
    trace(reinterpret_cast<void*>(get_data));
    pa_data_loader dl(get_data);
    PaStream *stream;
    pa_call(Pa_OpenStream(
            &stream,
            nullptr,
            &pa_params,
            SAMPLE_FREQUENCY,
            BUFFER_LEN,
            paClipOff,
            pa_callback,
            &dl));

    trace("playing");
    playing = true;
    pa_call(Pa_SetStreamFinishedCallback(stream, &pa_finished));
    pa_call(Pa_StartStream(stream));
    unique_lock<mutex> lock(mtx);
    trace("waiting");
    is_done.wait(lock, pa_done_playing);
    trace("done");
    pa_call(Pa_StopStream(stream));
    pa_call(Pa_CloseStream(stream));
}
#endif


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

    */

#endif
}

void pcm_close() {
#ifdef __linux__
    snd_pcm_close(handle);
#else
    Pa_Terminate();
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



