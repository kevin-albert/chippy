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

namespace controller {
    project current_project;
    instrument *instruments = current_project.instruments;
    sequence *sequences = current_project.sequences;
    expr<float> expressions[8];
    expr_context ctx;
    int current_step {-1};

    static volatile bool done {true};
    static volatile bitset<4> enabled_sequences;

    static void play_note_loop(const int instrument, const int note);
    static void play_sequence_loop(const int sequence);
    static void dj_loop();

    static thread audio_thread;


    void init() {
        ctx.define("sin", synth::sin);
        ctx.define("saw", synth::saw);
        ctx.define("sqr", synth::sqr);
        ctx.define("sin_t", synth::sin_t);
        ctx.define("saw_t", synth::saw_t);
        ctx.define("sqr_t", synth::sqr_t);
        ctx.define("sin_tq", synth::sin_tq);
        ctx.define("saw_tq", synth::saw_tq);
        ctx.define("sqr_tq", synth::sqr_tq);
        ctx.define("root", synth::root);
        ctx.define("scale", synth::scale);
        ctx.define("mix", synth::mix);
        ctx.define("env_t", synth::env_t); 
        ctx.define("env_tq", synth::env_tq); 
        pcm_open();
    }


    void setup_instrument(int id, const string &ex) {
        if (id < 0 || id >= 8) {
            throw invalid_argument("invalid track index");
        }
        expressions[id] = expr<float>(ctx, ex);
        instruments[id].enabled = true;
    }


    void remove_instrument(int id) {
        instruments[id] = instrument();
        expressions[id] = expr<float>();
    }


    void stop() {
        if (!done) {
            done = true;
            audio_thread.join();
        } 
        synth::reset_frame();
    }


    void play_note(const int instrument, const int note) {
        stop();
        done = false;
        audio_thread = thread(play_note_loop, instrument, note);
    }


    void play_note_loop(const int instrument, const int note) {

        evt_note n;
        n.start = 0;
        n.length = 16 * 4 * 32;
        n.start_note = n.end_note = note;
        n.start_vel = n.end_vel = 100;

        while (!done) {
            for (int i = 0; i < BUFFER_LEN; ++i) {
                if (synth::set_note(n) == 1) {
                    synth::reset_frame();
                }
                synth::incr_frame(60);
                synth::set_note(n);
                float val = (float) instruments[instrument].volume / 100 * 
                                    expressions[instrument].eval();
                audio_buffer[i] = val > 1 ? 0x80 : val < -1 ? 0x0 : 
                                  (uint8_t) (0x40 + (val+1)*64);
            }
            pcm_write();
        }
    }


    void write_blank_frames(int n, int &ptr) {
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
            if (done)
                break;
                
        }
    }


    void play_sequence(const int s_idx) {
        stop();
        done = false;
        audio_thread = thread(play_sequence_loop, s_idx);
    }

    void play_sequence_loop(const int s_idx) {
        sequence &s = sequences[s_idx];

        // iterate through the notes in sorted order
        s.sort();
        const int bpm = current_project.bpm;
        synth::incr_frame(bpm);
        
        // range of notes being played
        auto start = s.notes.begin(), end = start;
        auto last = s.notes.end();

        int ptr = 0;

        ofstream debug("/opt/chippy_files/debug.txt");

        while (start < last) {
            if (start >= end) {
                // wait until we have a note
                write_blank_frames(synth::frames_until(*start), ptr);
                end = start + 1;
            }

            debug << "start=" << (int) start->start_note << endl;

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
                debug << "end=" << (int) end->start_note << endl;
            }

            // sort the current range according to note end
            sort(start, end, [](const evt_note &n1, const evt_note &n2) { 
                return n1.start + n1.length < n2.start + n2.length; 
            });

            // we're playing the last note
            if (note_time == 0) {
                debug << "(last note)\n";
                evt_note sample;
                const evt_note latest = *(end-1);
                sample.start = latest.start + latest.length;
                sample.length = 0;
                note_time = synth::frames_until(sample);
            } else {
                debug << "note time: " << note_time << endl;
            }

            debug << "playing for " << note_time << " frames\n";
            while (start < end) {
                float val = 0;
                for (auto itor = start; itor < end; ++itor) {
                    const evt_note n = *itor;
                    if (synth::set_note(n) == 0) {
                        debug << (int) n.start_note << " ";
                        val += (float) instruments[n.instrument].volume / 100 * 
                                       expressions[n.instrument].eval();
                    } else {
                        debug << "done with " << (int) itor->start_note << endl;
                        start = itor + 1;
                    } 
                }
                audio_buffer[ptr] = val > 1 ? 0x80 : val < -1 ? 0x0 : 
                                    (uint8_t) (0x40 + (val+1)*64);
                if (ptr == BUFFER_LEN) {
                    if (done) 
                        return;

                    pcm_write();
                    ptr = 0;
                }
                debug << endl;
                synth::incr_frame(current_project.bpm);
                if (--note_time < 0)
                    break;
            }
        }

        int sequence_length = min(s.length, s.natural_length);
        evt_note dummy;
        dummy.start = sequence_length * s.ts;
        write_blank_frames(synth::frames_until(dummy), ptr);
        if (ptr > 0) {
            write_blank_frames(BUFFER_LEN-ptr, ptr);
        }
        debug << "done\n";
        debug.close();
        done = true;
    }

    void eval_with_params(int id, int note, double seconds, int length, float *buffer) {
        stop();
        if (id < 0 || id >= 8) {
            throw invalid_argument("invalid track index");
        }
        if (!instruments[id].enabled) {
            return;
        }
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
        stop();
        pcm_close();
    }
}

