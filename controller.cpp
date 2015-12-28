#include <string>
#include <cmath>
#include <bitset>
#include <thread>

using namespace std;
#include "project.h"
#include "expr.h"
#include "pcm_wrapper.h"

#define PLAY_NONE       0
#define PLAY_NOTE       1
#define PLAY_SEQUENCE   2
#define PLAY_PROJECT    3

project current_project;
instrument *instruments = current_project.instruments;
sequence *sequences = current_project.sequences;
expr<float> expressions[8];
expr_context ctx;

static volatile int play_mode {PLAY_NONE};
static volatile bool done {false};

namespace play_note {
    volatile int instrument {0};
    volatile int note {0};
};

namespace play_sequence {
    volatile int sequence {0};
};

namespace play_project {
    volatile bitset<4> enabled_sequences;
}

static void audio_loop(void);
static thread audio_thread(audio_loop);

static bool is_init {false};

static double t {0};
static double f {0};
static double w {0};
static double nt {0};
static double nl {0};
static uint32_t frame {0};


static void set_frequency(int note) {
    if (note < 1) f = 13.75;
    else {
        f = 13.75 * pow(2, (double) note / 12);
    }
}

float sin_h(const float harmonic) {
    return sin(harmonic * w);
}

float saw_h(const float harmonic) {
    float x = harmonic * w / 2;
    return 2 * (x - (int) x) - 1;
}

float sqr_h(const float harmonic) {
    return sin_h(harmonic) > 0 ? 1 : -1;
}

float sin_t(const float multiplier) {
    return sin(t * multiplier * M_PI * 2);
}

float sqr_t(const float multiplier) {
    return sin_t(multiplier) > 0 ? 1 : -1;
}

float saw_t(const float multiplier) {
    float x = t * multiplier / 2;
    return 2 * (x - (int) x) - 1;
}

float fastroot(const float v) {
    if (v == 0) return 0;
    if (v < 0) return -fastroot(-v);
    const float xhalf = 0.5f * v;
    union {
       float x;
       int i;
    } u;
    u.x = v;
    u.i = 0x5f3759df - (u.i >> 1);
    return v * u.x * (1.5f - xhalf * u.x * u.x);
}

float env(float attack, float fade) {
    float v = 1;
    if (attack > 0 && nt < attack) {
        v = nt / attack;
    }
    if (nt > nl - fade) {
        float v2 = (nl - fade) / (nl - nt);
        if (v2 < v) return v2;
    }
    return v;
}

float mix(float a, float b, float x) {
    if (x <= 0) return a;
    if (x >= 1) return b;
    return a * (1.0 - x) + b * x;
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


void controller_stop() {
    done = true;
    audio_thread.join();
}


void audio_loop() {
    int last_playmode = PLAY_NONE;
    ctx.define("sin", sin_h);
    ctx.define("saw", saw_h);
    ctx.define("sqr", sqr_h);
    ctx.define("t_sin", sin_t);
    ctx.define("t_saw", saw_t);
    ctx.define("t_sqr", sqr_t);
    ctx.define("root", fastroot);
    ctx.define("env", env); 
    ctx.define("mix", mix);
    pcm_open();
    int play_mode_last = PLAY_NONE;
    while (!done) {
        switch (play_mode) {
            case PLAY_NONE:
                this_thread::sleep_for(chrono::milliseconds(100));
                break;
            case PLAY_NOTE:
                if (play_mode_last != play_mode) {
                    t = 0;
                    frame = 0;
                }
                set_frequency(play_note::note);
                for (int i = 0; i < BUFFER_LEN; ++i) {
                    t = (double) frame / SAMPLE_FREQUENCY;
                    w = t * f * M_PI * 2;
                    float val = 0;
                    for (int j = 0; j < 4; ++j) {
                        if (instruments[j].enabled) {
                            float volume = instruments[j].volume;
                            val += volume * expressions[i].eval();
                        }
                    }
                    audio_buffer[i] = val > 1 ? 0x80 : val < -1 ? 0x0 : 
                        (uint8_t) (0x40 + (val+1)*64);
                    ++frame;
                }
                pcm_write();
                break;

        }

        play_mode_last = play_mode;
    }
    pcm_close();
}


void go() {
    
    f = 220;
    for (int n = 0; n < 10; ++n) {
        for (int i = 0; i < BUFFER_LEN; ++i) {
            t = (double) frame / SAMPLE_FREQUENCY;
            f = 220 + 220 * (frame / SAMPLE_FREQUENCY);
            w = t * f * M_PI * 2;
            float val = 0;
            for (int j = 0; j < 4; ++j) {
                if (instruments[j].enabled) {
                    float volume = instruments[j].volume;
                    val += volume * expressions[i].eval();
                }
            }
            audio_buffer[i] = val > 1 ? 0x80 : val < -1 ? 0x0 : (uint8_t) (0x40 + (val+1)*64);
            ++frame;
        }
        pcm_write();
    }
    pcm_close();
}


void eval_with_params(int id, int note, double seconds, int length, float *buffer) {
    if (id < 0 || id >= 8) {
        throw invalid_argument("invalid track index");
    }
    if (!instruments[id].enabled) return;
    set_frequency(note);
    for (int i = 0; i < length; ++i) {
        t = i * seconds / length;
        w = t * f * M_PI * 2;
        buffer[i] = expressions[id].eval();
    }
}


