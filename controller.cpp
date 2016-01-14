#include <string>
#include <cmath>
#include <bitset>
#include <chrono>
#include <algorithm>
#include <fstream>

using namespace std;
#include "project.h"
#include "expr.h"
#include "pcm_wrapper.h"
#include "synth_api.h"
#include "util.h"

namespace controller {
    project current_project;
    instrument *instruments = current_project.instruments;
    sequence *sequences = current_project.sequences;
    expr<float> expressions[NUM_INSTRUMENTS];
    expr_context ctx;
    int current_step {-1};
    SAMPLE last_sample {GROUND};

    expr<float> default_instrument;

    /*
    inline uint8_t f2u8(float value) {
        return value > 1 ? 0xff : value < -1 ? 0 : 128 + value * 127;
    }
    */

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
        ctx.func("noise",   synth::noise);
        ctx.func("sin_t",   synth::sin_t);
        ctx.func("saw_t",   synth::saw_t);
        ctx.func("sqr_t",   synth::sqr_t);
        ctx.func("sin_tq",  synth::sin_tq);
        ctx.func("saw_tq",  synth::saw_tq);
        ctx.func("sqr_tq",  synth::sqr_tq);
        ctx.func("root",    synth::root);
        ctx.func("pow",     synth::pow);
        ctx.func("min",     synth::min);
        ctx.func("max",     synth::max);
        ctx.func("scale",   synth::scale);
        ctx.func("scale01", synth::scale01);
        ctx.func("mix",     synth::mix);
        ctx.func("env",     synth::env_tq); 
        ctx.func("env_t",   synth::env_t); 
        pcm_open();
        default_instrument = expr<float>(ctx, "mix(sin(1), sqr(2), 1.0 / (nl * 4 + 1))");
    }


    void setup_instrument(int id, const string &ex) {
        expressions[id] = expr<float>(ctx, ex);
        instruments[id].enabled = true;
    }


    void remove_instrument(int id) {
        instruments[id] = instrument();
        expressions[id] = expr<float>();
    }


    namespace note_player {
        evt_note n;
        expr<float> *inst;
        float volume;
        float pan;
        bool (*condition)();

        bool get_note_data(SAMPLE *buffer, uint32_t buffer_len) {
            if (condition()) {
                for (int i = 0; i < buffer_len; ++i) {
                    if (synth::set_note(n) == 1) {
                        synth::reset_frame();
                    }
                    synth::incr_frame(60);
                    synth::set_note(n);
                    float value = volume * inst->eval();

                    *buffer++ = value * (1.0f - pan);
                    *buffer++ = value * pan;
                }
                return true;
            } else {
                trace("done");
                return false;
            }
        }

        void go(const int instrument, const int note, bool (condition())) {
            synth::reset_frame();
            note_player::condition = condition;

            n.start = 0;
            n.length = 16 * 4 * 32;
            n.start_note = n.end_note = note;
            n.start_vel = n.end_vel = 100;
            pan = (float) (PAN_RIGHT - n.pan) / 0xff;
            trace("pan left: " << (1.0f - pan));
            trace("pan right: " << (pan));

            inst = instruments[instrument].enabled ?
                   &expressions[instrument] : &default_instrument;

            volume = ((float) current_project.volume / 100) *
                     ((float) instruments[instrument].volume / 100);

            trace("data function: " << reinterpret_cast<void*>(get_note_data));
            pcm_play(get_note_data);
        }
    };

    void play_note(const int instrument, const int note, bool (condition())) {
        note_player::go(instrument, note, condition); 
    }


    namespace sequence_player {
        sequence *s;
        int bpm;
        float volume;
        vector<evt_note>::iterator start;
        vector<evt_note>::iterator end;
        vector<evt_note>::iterator last;
        bool (*condition)();


        bool get_data(SAMPLE *buffer, uint32_t buffer_len) {
            int ptr = 0;

            if (start >= last) {
                s->sort();
                start = s->notes.begin(), end = start;
                last = s->notes.end();
            }
            
            // get the notes that belong to this buffer
            // play them

            return false;
        }

        void go(const int s_idx, bool(condition())) {

            synth::reset_frame();
            sequence_player::condition = condition;
            s = &sequences[s_idx];

            bpm = current_project.bpm;
            volume = (float) current_project.volume / 100;

            // force init to happen in first loop
            start = last = s->notes.end();
            current_step = 0;
            //pcm_play(get_data);
        }
    }

    void play_sequence(const int s_idx, bool(condition())) {
        /*
        synth::reset_frame();
        sequence &s = sequences[s_idx];

        // iterate through the notes in sorted order
        const int bpm = current_project.bpm;
        synth::incr_frame(bpm);
        float volume = (float) current_project.volume / 100;
        int ptr = 0;
        
        while (condition()) {

            s.sort();
            
            // range of notes being played
            auto start = s.notes.begin(), end = start;
            auto last = s.notes.end();

            while (start < last) {
                if (start >= end) {
                    // wait until we have a note
                    if (!write_blank_frames(synth::frames_until(*start), ptr, condition)) {
                        return;
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
                            val += ((float) instruments[n.instrument].volume / 100) * 
                                   expressions[n.instrument].eval();
                        } else {
                            start = itor + 1;
                        } 
                    }

                    audio_buffer[ptr++] = last_sample = f2u8(volume * val); 
                    if (ptr == BUFFER_LEN) {
                        if (!condition()) {
                            goto play_sequence_done;
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
                evt_note dummy;
                dummy.start = s.length * s.ts;
                if (!write_blank_frames(synth::frames_until(dummy), ptr, condition)) {
                    return;
                }
            }
        }

play_sequence_done:
        write_blank_frames(BUFFER_LEN-ptr, ptr, condition);
        */
    }

    void eval_with_params(int id, int note, double seconds, int length, float *buffer) {
        if (!instruments[id].enabled) {
            return;
        }
        synth::reset_frame();
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
    }

    void destroy() {
        pcm_close();
    }
}


