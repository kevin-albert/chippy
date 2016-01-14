#ifndef pcm_wrapper_h
#define pcm_wrapper_h

#include <iostream>
#include <cstdlib>

typedef float SAMPLE;
#define GROUND 0

inline SAMPLE to_sample(float f) { return f; }


#ifdef __linux__
  #define SAMPLE_FREQUENCY 48000
  #define BUFFER_LEN (4800 * 3)
#else
  #define SAMPLE_FREQUENCY 48000
  #define BUFFER_LEN 1024
#endif


void pcm_open(void);
void pcm_write(void);
void pcm_close(void);
void pcm_write_wav(ostream &out, const uint8_t *data, const uint32_t data_size);
void pcm_play(bool (get_data(SAMPLE*, uint32_t)));


#endif
