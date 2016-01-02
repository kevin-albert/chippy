#include <cmath>
#include <fstream>

using namespace std;
#include "synth_api.h"
#include "pcm_wrapper.h"
#include "util.h"

namespace synth {

    bool first;
    int frame {0};
    double bps {120};
    double t {0};
    double tq {0};
    double f {0};
    double w {0};
    double nl {0};
    double nlq {0};
    double nt {0};
    double ntq {0};

    void incr_frame(const int bpm) {
        ++frame;
        t = (double) frame / SAMPLE_FREQUENCY;
        bps = (double) bpm / 60;
        tq = t * bps;
    }

    void reset_frame() {
        frame = -1;
        t = 0;
        tq = 0;
    }

    int frames_until(const evt_note &n) {
        int note_start_frame = (SAMPLE_FREQUENCY * n.start) / 16 / bps;
        if (frame < note_start_frame) {
            return note_start_frame - frame;
        }

        int note_end_frame = (SAMPLE_FREQUENCY * (n.start + n.length)) / 16 / bps;
        if (frame > note_end_frame) {
            return note_end_frame - frame;
        }

        return 0;
    }


    int set_note(const evt_note &n) {

        uint32_t note_start_frame = (SAMPLE_FREQUENCY * n.start) / 16 / bps;
        if (frame < note_start_frame) {
            return -1;
        }

        uint32_t note_end_frame = (SAMPLE_FREQUENCY * (n.start + n.length)) / 16 / bps;
        if (frame > note_end_frame) {
            return 1;
        }

        double note_progress = (double) (frame - note_start_frame) / (note_end_frame - note_start_frame);
        trace("note progress=" << note_progress);
        double note_value = (double) n.start_note * (1.0 - note_progress) +
                            (double) n.end_note * note_progress;

        if (note_value < 1) note_value = 1;

        f = 13.75 * pow(2, note_value / 12);
        w = f * t * M_PI * 2;
        nt = (double) (frame - note_start_frame) / SAMPLE_FREQUENCY;
        ntq = nt * bps;
        nlq = n.length / 16;
        nl = nlq / bps;

        return 0;
    }


    float sin(const float harmonic) {
        return ::sin(harmonic * w);
    }

    float saw(const float harmonic) {
        float x = harmonic * t * f / 2;
        return 2 * (x - (int) x) - 1;
    }

    float sqr(const float harmonic) {
        return sin(harmonic) > 0 ? 1 : -1;
    }

    float sin_t(const float multiplier) {
        return ::sin(t * multiplier * M_PI * 2);
    }

    float sqr_t(const float multiplier) {
        return sin(multiplier) > 0 ? 1 : -1;
    }

    float saw_t(const float multiplier) {
        float x = t * multiplier / 2;
        return 2 * (x - (int) x) - 1;
    }

    float sin_tq(const float multiplier) {
        return ::sin(tq * multiplier * M_PI * 2);
    }

    float sqr_tq(const float multiplier) {
        return sin_tq(multiplier) > 0 ? 1 : -1;
    }

    float saw_tq(const float multiplier) {
        float x = tq * multiplier / 2;
        return 2 * (x - (int) x) - 1;
    }

    float root(const float v) {
        if (v == 0) return 0;
        if (v < 0) return -root(-v);
        const float xhalf = 0.5f * v;
        union {
           float x;
           int i;
        } u;
        u.x = v;
        u.i = 0x5f3759df - (u.i >> 1);
        return v * u.x * (1.5f - xhalf * u.x * u.x);
    }

    float env_t(const float attack, const float fade) {
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

    float env_tq(const float attack, const float fade) {
        float v = 1;
        if (attack > 0 && ntq < attack) {
            v = ntq / attack;
        }
        if (ntq > nlq - fade) {
            float v2 = (nlq - fade) / (nlq - ntq);
            if (v2 < v) return v2;
        }
        return v;
        
    }

    float mix(const float a, const float b, const float x) {
        if (x <= 0) return a;
        if (x >= 1) return b;
        return a * (1.0 - x) + b * x;
    }

    float scale(const float lower_bound_from, const float upper_bound_from,
                const float lower_bound_to, const float upper_bound_to,
                const float value) {
        return value * (upper_bound_to - lower_bound_to) / (upper_bound_from - lower_bound_from)
                     + lower_bound_to - lower_bound_from;
    }

    float scale01(const float lower_bound_from, const float upper_bound_from,
                  const float value) {
        return scale(lower_bound_from, upper_bound_from, 0, 1, value);
    }
}


