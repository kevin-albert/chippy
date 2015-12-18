#include <iostream>
#include <string>

using namespace std;
#include "controller.h"

int main(void) {
    setup_instrument(0, 0, "sin()");
    go();
}

