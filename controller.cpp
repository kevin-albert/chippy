#include <vector>
#include <cmath>

using namespace std;
#include "controller.h"
#include "expr.h"
#include "pcm_wrapper.h"

class track {
    public:
        track() {}
        void add_instrument(int, const string&);
        expr<uint8_t> &operator[](size_t idx) {
            return instruments[idx];
        }
        float volume {1};
    private:
        expr<uint8_t> instruments[4];
};

track tracks[8];
expr_context ctx;
bool is_init {false};

double t {0};
double f {0};

uint8_t w_sin() {
    return 0xf0 * (1 + sin(t * f));
}

uint8_t w_saw() {
    return 0xff * ((t * f) - (int) (t * f));
}

uint8_t w_sqr() {
    return w_sin() > 0xf0 ? 0xff : 0x0;
}

void init() {
    if (!is_init) {
        ctx.add_thing("sin", "uint8_t", w_sin);
        ctx.add_thing("saw", "uint8_t", w_saw);
        ctx.add_thing("sqr", "uint8_t", w_sqr);
        is_init = true;
    }
}

void track::add_instrument(int inst, const string &ex) {
    init();
    if (inst < 0 || inst > 4) {
        throw invalid_argument("invalid instrument index");
    }
    instruments[inst] = expr<uint8_t>(ctx, ex);
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
    f = 440;
    for (int i = 0; i < 200; ++i) {
        uint8_t val = 0xf0;
        for (int j = 0; j < 8; ++j) {
            for (int k = 0; k < 4; ++k) {
                val += (tracks[j][k].eval() - 0xf0) * tracks[j].volume;
            }
        }
        buffer[i] = val;
        t += 0.1;
    }
    pcm_write(buffer, 200);
}


