#ifndef controller_h
#define controller_h

#include <string>
#include "expr.h"
#include "project.h"

extern project current_project;
extern instrument *instruments;
extern sequence *sequences;
extern expr<float> expressions[4];
extern expr_context context;

void setup_instrument(int track, const string &ex);
void remove_instrument(int track);
void eval_with_params(int track, int note, double seconds, int length, float *buffer);
void enable_sequence(bool);

void play_single(int track, int note, double seconds);
void play();
void stop();

#endif
