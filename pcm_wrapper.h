#ifndef pcm_wrapper_h
#define pcm_wrapper_h

#include <iostream>
#include <cstdlib>
#ifdef __linux__
  #define SAMPLE_FREQUENCY 48000
  #define BUFFER_LEN 1024
#else
  #define SAMPLE_FREQUENCY 48000
  #define BUFFER_LEN 100
#endif


extern uint8_t audio_buffer[BUFFER_LEN];

void pcm_open(void);
void pcm_write(void);
void pcm_close(void);
void pcm_write_wav(ostream &out, const uint8_t *data, const uint32_t data_size);


#endif
