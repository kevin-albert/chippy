#include <iostream>

using namespace std;
#include "pcm_wrapper.h"

void pcm_write(uint8_t *buffer, int length) {
    for (int i = 0; i < length; ++i) {
        cout << "+";
        for (int j = 0; j < buffer[i]; j += 2) {
            cout << " ";
        }
        cout << ".\n";
    }
}

