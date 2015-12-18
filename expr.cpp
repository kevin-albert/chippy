#include <string>
#include <stdio.h>

using namespace std;
#include "expr.h"

static int _id = 0xfa;
int expr_id() {
    return _id++;
}

void expr_context::write(FILE *out) {
    for (string &line : lines) {
        fprintf(out, "%s\n", line.c_str());
    }
}

string demangle(const string &name) {
    char *type_name = abi::__cxa_demangle(name.c_str(), 0, 0, 0);
    string type_name_str = type_name;
    free(type_name);
    return type_name_str;
}

void print_types(ostream &out, initializer_list<type_index> types) {
    string comma = "";
    for (const type_index &idx : types) {
        out << comma << demangle(idx.name());
        comma = ",";
    }
}

