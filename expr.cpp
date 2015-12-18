#include <string>
#include <stdio.h>

using namespace std;
#include "expr.h"

static int _id = 0xfa;
int expr_id() {
    return _id++;
}

void expr_context::write(FILE *out) {
    for (size_t i = 0; i < names.size(); ++i) {
        fprintf(out, "static %s(*%s)() = (%s(*)()) %p;\n", 
                types[i].c_str(), names[i].c_str(), types[i].c_str(), values[i]);
        printf("static %s(*%s)() = (%s(*)()) %p;\n", 
                types[i].c_str(), names[i].c_str(), types[i].c_str(), values[i]);
        //fprintf(out, "#define %s (hack_%s())\n", names[i].c_str(), names[i].c_str());
    }
}
