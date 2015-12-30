#ifndef pcm_wrapper_h
#define pcm_wrapper_h

#include <cstdlib>
#ifdef __linux__
  #define SAMPLE_FREQUENCY 48000
  #define BUFFER_LEN 48000
#else
  #define SAMPLE_FREQUENCY 1000
  #define BUFFER_LEN 1000
#endif

extern uint8_t audio_buffer[BUFFER_LEN];

void pcm_open(void);
void pcm_write(void);
void pcm_close(void);

#endif
