#include <iostream>
#include <string>
#include <cstdio>
#include <dlfcn.h>

using namespace std;
#include "expr.h"

double current_frequency() {
    return 440;    
}

double current_time() {
    return 1.1;
}

int main(void) {
    try {
        expr_context ctx;
        ctx.add_thing("t", "double", (void*(*)) &current_time);
        ctx.add_thing("f", "double", (void*(*)) &current_frequency);
        expr<float> ex1(ctx, "t * f");
        expr<int> ex2(ctx, "1 << 31");
        cout << "ex1: " << ex1.eval() << endl
             << "ex2: " << ex2.eval() << endl;
        return 0;
    } catch (runtime_error &ex) {
        cerr << "fail: " << ex.what() << endl;
        return 1;
    }
}
