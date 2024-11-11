// example.c
#include <stdio.h>

const char message[] = "Hello from ELF!";
int initialized_var = 42;
int uninitialized_var;

void print_message() {
    printf("%s\n", message);
}

int main() {
    uninitialized_var = initialized_var;
    print_message();
    return 0;
}
