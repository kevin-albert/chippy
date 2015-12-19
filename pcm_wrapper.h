#ifndef pcm_wrapper_h
#define pcm_wrapper_h

#include <cstdlib>
#define SAMPLE_FREQUENCY 48000
#ifdef __linux__
  #define BUFFER_LEN 4800
#else
  #define BUFFER_LEN 4800
#endif

extern uint8_t audio_buffer[BUFFER_LEN];

void pcm_open(void);
void pcm_write(void);
void pcm_close(void);

#endif
