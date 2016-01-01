#ifndef controller_h
#define controller_h

#include <string>
#include "expr.h"
#include "project.h"

namespace controller {
    extern project current_project;
    extern instrument *instruments;
    extern sequence *sequences;
    extern expr<float> expressions[NUM_INSTRUMENTS];
    extern int current_step; // playhead location in sixteenth notes
    extern volatile bool playing;

    void setup_instrument(int track, const string &ex);
    void remove_instrument(int track);
    void eval_with_params(int track, int note, double seconds, int length, float *buffer);
    void enable_sequence(bool);
    void init();
    void destroy();

    void play_note(const int instrument, const int note, void (cb()));
    void play_sequence(const int sequence, void (cb()));
    void play(const void (cb()));
    void stop();
};

#endif
