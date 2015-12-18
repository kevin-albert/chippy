#include <iostream>
#include <string>

using namespace std;
#include "controller.h"

int main(void) {
    setup_instrument(0, 0, "sin(1) / 2 + sin(2) / 4");
    setup_instrument(0, 1, "sqr(1) * 0.2");
    go();
}

