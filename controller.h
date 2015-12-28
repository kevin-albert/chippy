#ifndef controller_h
#define controller_h

#include <string>
#include "expr.h"
#include "project.h"

extern project current_project;
extern instrument *instruments;
extern sequence *sequences;
extern expr<float> expressions[8];

void setup_instrument(int track, const string &ex);
void remove_instrument(int track);
void eval_with_params(int track, int note, double seconds, int length, float *buffer);
void enable_sequence(bool);
void controller_stop();

void play_note(int instrument, int note);
void play_sequence(int sequence);
void play();
void stop();

#endif
