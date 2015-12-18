#include <string>
#include <cmath>

using namespace std;
#include "controller.h"
#include "expr.h"
#include "pcm_wrapper.h"

class track {
    public:
        track() {}
        void add_instrument(int, const string&);
        expr<float> &operator[](size_t idx) {
            return instruments[idx];
        }
        float volume {1};
    private:
        expr<float> instruments[4];
};

track tracks[8];
expr_context ctx;
bool is_init {false};

static double t {0};
static double f {0};
static double w {0};

float w_sin(float m) {
    return sin(w * m);
}

float w_saw(float m) {
    return (w * m) - (int) (w * m);
}

float w_sqr(float m) {
    return w_sin(m) > 0 ? 1 : -1;
}

void init() {
    if (!is_init) {
        ctx.define("sin", w_sin);
        ctx.define("saw", w_saw);
        ctx.define("sqr", w_sqr);
        is_init = true;
    }
}

void track::add_instrument(int inst, const string &ex) {
    init();
    if (inst < 0 || inst > 4) {
        throw invalid_argument("invalid instrument index");
    }
    instruments[inst] = expr<float>(ctx, ex);
}

void setup_instrument(int track, int inst, const string &ex) {
    if (track < 0 || track >= 8) {
        throw invalid_argument("invalid track index");
    }
    tracks[track].add_instrument(inst, ex);
}

void go() {
    init();
    cout << "go\n";
    uint8_t buffer[200];
    f = 220;
    for (int i = 0; i < 200; ++i) {
        w = t * f * M_PI * 2;
        float val = 0;
        for (int j = 0; j < 8; ++j) {
            float volume = tracks[j].volume;
            for (int k = 0; k < 4; ++k) {
                val += tracks[j][k].eval();
            }
        }
        buffer[i] = val > 1 ? 0xff : val < -1 ? 0x0 : (uint8_t) (0x80 * (val+1));
        t += 0.0001;
    }
    pcm_write(buffer, 200);
}


