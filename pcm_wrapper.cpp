#include <iostream>

using namespace std;
#include "pcm_wrapper.h"

void pcm_write(uint8_t *buffer, int length) {
    for (int i = 0; i < length; ++i) {
        for (uint8_t j = 0; j < buffer[i] / 4; ++j) {
            cout << " ";
        }
        cout << ".\n";
    }
}

