#include <iostream>
#include <string>

using namespace std;
#include "track.h"

ostream &operator<<(ostream &output, const track &t) {
    output << t.volume << " " << t.enabled << " " << (t.enabled ? t.instrument : "0");
    return output;
}


istream &operator>>(istream &input, track &t) {
    input >> t.volume >> t.enabled >> t.instrument;
    return input;
}

