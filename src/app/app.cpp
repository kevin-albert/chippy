#include "mem/mem.h"
#include <iostream>
#include <cstring>

using namespace Mem;
int main(void) {
    Instance<char[10]> str = makeInstance<char[10]>("test", "str");
    memcpy(*str, "hello", 6);
    closeInstance(str);
}
