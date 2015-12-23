#include <stdio.h>

void foo();

void (*bar())() {
    return foo;   
}

void (*(*baz())())() {
    return bar;
}

int main(void) {
    void (*(*g)())() = baz();
    void (*f)() = g();
    f();
}

void foo() {
    printf("hello!\n");
}


