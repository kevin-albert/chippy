#include <string>
#include <cmath>
#include <bitset>
#include <thread>
#include <chrono>
#include <algorithm>
#include <fstream>

using namespace std;
#include "project.h"
#include "expr.h"
#include "pcm_wrapper.h"
#include "synth_api.h"
#include "io.h"

namespace controller {
    project current_project;
    instrument *instruments = current_project.instruments;
    sequence *sequences = current_project.sequences;
    expr<float> expressions[NUM_INSTRUMENTS];
    expr_context ctx;
    int current_step {-1};

    volatile bool playing   {false};
    volatile bool available {true};

    void play_note_loop(const int instrument, const int note, void (cb()));
    void play_sequence_loop(const int sequence, void (cb()));
    void dj_loop(void (cb()));
    thread audio_thread;

    expr<float> default_instrument;

    void init() {
        ctx.var("t",       &synth::t);
        ctx.var("f",       &synth::f);
        ctx.var("tq",      &synth::tq);
        ctx.var("w",       &synth::w);
        ctx.var("nl",      &synth::nl);
        ctx.var("nlq",     &synth::nlq);
        ctx.var("nt",      &synth::nt);
        ctx.var("ntq",     &synth::ntq);
        ctx.func("sin",     synth::sin);
        ctx.func("saw",     synth::saw);
        ctx.func("sqr",     synth::sqr);
        ctx.func("sin_t",   synth::sin_t);
        ctx.func("saw_t",   synth::saw_t);
        ctx.func("sqr_t",   synth::sqr_t);
        ctx.func("sin_tq",  synth::sin_tq);
        ctx.func("saw_tq",  synth::saw_tq);
        ctx.func("sqr_tq",  synth::sqr_tq);
        ctx.func("root",    synth::root);
        ctx.func("scale",   synth::scale);
        ctx.func("mix",     synth::mix);
        ctx.func("env",     synth::env_tq); 
        ctx.func("env_t",   synth::env_t); 
        pcm_open();
        default_instrument = expr<float>(ctx, "mix(saw(1), sqr(1), nl * 4 + 0.5)");
        trace("done");
    }


    void setup_instrument(int id, const string &ex) {
        expressions[id] = expr<float>(ctx, ex);
        instruments[id].enabled = true;
    }


    void remove_instrument(int id) {
        instruments[id] = instrument();
        expressions[id] = expr<float>();
    }


    void stop() {
        trace("enter");
        if (!available) {
            trace("available = false, joining audio_thread");
            playing = false;
            audio_thread.join();
        } 
        synth::reset_frame();
        available = true;
        playing = false;
        trace("available = true, playing = false");
    }

    void reset() {
        trace("enter");
        stop();
        available = false;
        playing = true;
        trace("available = false, playing = true");
    }

    void done() {
        available = true;
        playing = false;
        trace("available = true, playing = false");
    }

    void play_note(const int instrument, const int note, void (cb())) {
        trace("resetting...");
        reset();
        audio_thread = thread(play_note_loop, instrument, note, cb);
        trace("started play_note_loop");
    }


    void play_note_loop(const int instrument, const int note, void (cb())) {
        
        evt_note n;
        n.start = 0;
        n.length = 16 * 4 * 32;
        n.start_note = n.end_note = note;
        n.start_vel = n.end_vel = 100;

        expr<float> &inst = instruments[instrument].enabled ?
            expressions[instrument] : default_instrument;

        while (playing) {
            trace("generating samples");
            for (int i = 0; i < BUFFER_LEN; ++i) {
                if (synth::set_note(n) == 1) {
                    synth::reset_frame();
                }
                synth::incr_frame(60);
                synth::set_note(n);
                float val = (float) instruments[instrument].volume / 100 * inst.eval();
                audio_buffer[i] = val > 1 ? 0x80 : val < -1 ? 0x0 : 
                                  (uint8_t) (0x40 + (val+1)*64);
            }
            trace("writing samples");
            pcm_write();
            trace("playing = " << playing);
        }
        
        trace("exited loop\n");

        cb();
        done();
        trace("play_note_loop finished");
    }


    bool write_blank_frames(int n, int &ptr) {
        while (n > 0) {
            int end = min(BUFFER_LEN, ptr + n);
            int start = ptr;
            while (ptr < end) {
                audio_buffer[ptr++] = 0x40;
                synth::incr_frame(current_project.bpm);
            }
            if (ptr == BUFFER_LEN) {
                pcm_write();
                ptr = 0;
            }
            n -= end - start;
            if (!playing)
                return false;
                
        }

        return true;
    }


    void play_sequence(const int s_idx, void (cb())) {
        trace("resetting...");
        reset();
        audio_thread = thread(play_sequence_loop, s_idx, cb);
        trace("started play_sequence_loop");
    }

    void play_sequence_loop(const int s_idx, void (cb())) {
        sequence &s = sequences[s_idx];

        // iterate through the notes in sorted order
        s.sort();
        const int bpm = current_project.bpm;
        synth::incr_frame(bpm);
        
        // range of notes being played
        auto start = s.notes.begin(), end = start;
        auto last = s.notes.end();
        int ptr = 0;

        while (start < last) {
            if (start >= end) {
                // wait until we have a note
                if (!write_blank_frames(synth::frames_until(*start), ptr)) {
                    goto sequence_loop_done;
                }
                end = start + 1;
            }

            // find all other notes playing at this time
            int note_time = 0;
            if (end != last) {
                while (note_time == 0) {
                    note_time = synth::frames_until(*end);
                    if (note_time == 0) {
                        if (++end >= last)
                            break;
                    }
                }
            }

            // sort the current range according to note end
            sort(start, end, [](const evt_note &n1, const evt_note &n2) { 
                return n1.start + n1.length < n2.start + n2.length; 
            });

            // we're playing the last note
            if (note_time == 0) {
                evt_note sample;
                const evt_note latest = *(end-1);
                sample.start = latest.start + latest.length;
                sample.length = 0;
                note_time = synth::frames_until(sample);
            }

            while (start < end) {
                float val = 0;
                for (auto itor = start; itor < end; ++itor) {
                    const evt_note n = *itor;
                    if (synth::set_note(n) == 0) {
                        val += (float) instruments[n.instrument].volume / 100 * 
                                       expressions[n.instrument].eval();
                    } else {
                        start = itor + 1;
                    } 
                }

                audio_buffer[ptr] = val > 1 ? 0x80 : val < -1 ? 0x0 : 
                                    (uint8_t) (0x40 + (val+1)*64);
                if (ptr == BUFFER_LEN) {
                    if (!playing) {
                        goto sequence_loop_done;
                    }

                    pcm_write();
                    ptr = 0;
                }
                synth::incr_frame(current_project.bpm);
                if (--note_time < 0)
                    break;
            }
        }

        {
            int sequence_length = min(s.length, s.natural_length);
            evt_note dummy;
            dummy.start = sequence_length * s.ts;
            if (!write_blank_frames(synth::frames_until(dummy), ptr)) {
                goto sequence_loop_done;
            }
            if (ptr > 0) {
                if (!write_blank_frames(BUFFER_LEN-ptr, ptr)) {
                    goto sequence_loop_done;
                }
            }
        }

sequence_loop_done:
        cb();
        done();
        trace("play_sequence_loop done");
    }

    void eval_with_params(int id, int note, double seconds, int length, float *buffer) {
        if (!instruments[id].enabled) {
            return;
        }
        
        reset();
        evt_note n;
        n.start = 0;
        n.length = 16;
        n.start_note = n.end_note = note;
        n.start_vel = n.end_vel = 100;
        for (int i = 0; i < length; ++i) {
            synth::incr_frame(current_project.bpm);
            synth::set_note(n);
            buffer[i] = expressions[id].eval();
        }
        done();
    }

    void destroy() {
        trace("started destroy()");
        stop();
        pcm_close();
        trace("controller destroyed");
    }
}


