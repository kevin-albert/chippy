#ifndef synth_api_h
#define synth_api_h

#include "project.h"

/**
 * API for binding runtime-compiled expressions to function pointers / values in
 * program memory
 */
namespace synth {

    extern int frame;       // current sample index
    extern double t;        // time in seconds
    extern double tq;       // time in quarter notes
    extern double w;        // t * f * 2π
    extern double nl;       // length of current note in seconds
    extern double nlq;      // length of current note in in quarter notes
    extern double nt;       // offset into current note in seconds
    extern double ntq;      // offset into current note in quarter notes
    extern double f;        // frequency

    // Sets the values of frame, t, tq
    void incr_frame(const int bpm);
    void reset_frame();

    // Sets the values of f, w, nt, ntq, nl, nlq
    // returns -1 if the current frame is before the given note
    //          0 if this note will be played
    //          1 if the current frame is beyond the given note
    int set_note(const evt_note &n);

    // find out how many frames we need to wait until n is played
    int frames_until(const evt_note &n);


    //
    // Oscillators
    // sin.* -> produce a sin wave from the given frequency
    // saw.* -> produce a saw wave from the given frequency
    // sqr.* -> produce a square wave from the given frequency
    //

    //
    // Oscillate over the current note * the given harmonic
    // 
    float sin(const float harmonic);
    float saw(const float harmonic);
    float sqr(const float harmonic);
    // create noise between -scale and scale
    float noise(const float scale);

    //
    // Oscillate over the current time in seconds * the given multiplier
    //
    float sin_t(const float multiplier);
    float sqr_t(const float multiplier);
    float saw_t(const float multiplier);

    //
    // Oscillate over the current time in quarter notes * the given multiplier
    //
    float sin_tq(const float multiplier);
    float sqr_tq(const float multiplier);
    float saw_tq(const float multiplier);


    //
    // Filters
    // Modify values
    //

    // Take the square root of a value
    float root(const float v);
    // Raise the given number to a power
    float pow(const float v, const float p);

    // Translate and scale a value from one range to another
    // example: scale(0,1, 2,4, 0.5) -> 3
    float scale(const float lower_bound_from, const float upper_bound_from,
                const float lower_bound_to, const float upper_bound_to,
                const float value);

    // Translate and scale one range to the range 0, 1
    // invokes scale(lower_bound_from, upper_bound_from, 0, 1, value)
    float scale01(const float lower_bound_from, const float upper_bound_from,
                  const float value);

    //
    // Mix values a and b based on the value of x (between 0 and 1)
    // If x is < 0, it is interpreted as 0
    // If x > 1, it is in interpreted as 1
    //
    float mix(const float a, const float b, const float x);

    //
    // Envelopes
    // Produce a value between 0 and 1 based on how long a note is being played,
    // timed either in seconds or in quarter notes
    //
    float env_t(const float attack, const float fade);
    float env_tq(const float attack, const float fade);
};

#endif
