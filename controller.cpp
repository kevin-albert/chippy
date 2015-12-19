#include <string>
#include <cmath>
#include <bitset>

using namespace std;
#include "controller.h"
#include "expr.h"
#include "pcm_wrapper.h"

track tracks[4];
expr<float> instruments[4];
expr_context ctx;

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

float sin_w(const float m) {
    return sin(w * m);
}

float saw_w(const float m) {
    return 2 * ((w * m) - (int) (w * m)) - 1;
}

float sqr_w(const float m) {
    return sin_w(m) > 0 ? 1 : -1;
}

float sin_default() {
    return sin(w);
}

float sqr_default() {
    return sin_default() > 0 ? 1 : -1;
}

float saw_default() {
    return 2 * (w - (int) w) - 1;
}

float sin_t(const float m) {
    return sin(t * m * M_PI * 2);
}

float sqr_t(const float m) {
    return sin_t(m) > 0 ? 1 : -1;
}

float saw_t(const float m) {
    return 2 * ((t * m) - (int) (t * m)) - 1;
}

float dummy_root(const float v) {
    return v;
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

static void init() {
    if (!is_init) {
        ctx.define("sin", sin_w);
        ctx.define("saw", saw_w);
        ctx.define("sqr", sqr_w);
        ctx.define("sinf", sin_default);
        ctx.define("sqrf", sqr_default);
        ctx.define("sawf", saw_default);
        ctx.define("sints", sin_t);
        ctx.define("sawts", saw_t);
        ctx.define("sqrts", sqr_t);
        ctx.define("root", fastroot);
        ctx.define("env", env); 
        is_init = true;
        pcm_open();
    }
}


void setup_instrument(int id, const string &ex) {
    init();
    if (id < 0 || id >= 4) {
        throw invalid_argument("invalid track index");
    }
    instruments[id] = expr<float>(ctx, ex);
    tracks[id].enabled = true;
}


void remove_instrument(int id) {
    tracks[id] = track();
    instruments[id] = expr<float>();
}


void go() {
    init();
    cout << "go\n";
    
    f = 220;
    for (int n = 0; n < 10; ++n) {
        for (int i = 0; i < BUFFER_LEN; ++i) {
            t = (double) frame / SAMPLE_FREQUENCY;
            f = 220 + 220 * (frame / SAMPLE_FREQUENCY);
            w = t * f * M_PI * 2;
            float val = 0;
            for (int j = 0; j < 4; ++j) {
                if (tracks[j].enabled) {
                    float volume = tracks[j].volume;
                    val += volume * instruments[i].eval();
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
    init();
    if (id < 0 || id >= 4) {
        throw invalid_argument("invalid track index");
    }
    if (!tracks[id].enabled) return;
    set_frequency(note);
    for (int i = 0; i < length; ++i) {
        t = i * seconds / length;
        w = t * f * M_PI * 2;
        buffer[i] = instruments[id].eval();
    }
}


