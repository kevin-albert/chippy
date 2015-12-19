#ifndef controller_h
#define controller_h

#include <string>
#include "track.h"
#include "expr.h"

extern track tracks[4];
extern expr<float> instruments[4];
extern expr_context context;

void setup_instrument(int track, const string &ex);
void remove_instrument(int track);
void eval_with_params(int track, int note, double seconds, int length, float *buffer);

#endif
