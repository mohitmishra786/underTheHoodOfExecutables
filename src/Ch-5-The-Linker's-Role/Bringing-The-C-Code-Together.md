# The Linker: Bringing The C Code Together

When you're building anything but the smallest C programs, you'll typically split your code across multiple files. The linker is the crucial tool that brings all these pieces together into a working executable. In this post, we'll explore how the linker works and how to use it effectively.

## A Multi-File Project Example

Let's start with a practical example that we'll use throughout this post. We'll create a simple calculator library:

```c
// mylib.h
#ifndef MYLIB_H
#define MYLIB_H

// Function declarations
int add(int a, int b);
int subtract(int a, int b);
int multiply(int a, int b);
int divide(int a, int b);

// Error handling
extern int last_error;
#define ERROR_DIVIDE_BY_ZERO 1

#endif
```

```c
// mylib.c
#include "mylib.h"

int last_error = 0;

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int multiply(int a, int b) {
    return a * b;
}

int divide(int a, int b) {
    if (b == 0) {
        last_error = ERROR_DIVIDE_BY_ZERO;
        return 0;
    }
    return a / b;
}
```

```c
// main.c
#include <stdio.h>
#include "mylib.h"

int main() {
    printf("10 + 5 = %d\n", add(10, 5));
    printf("10 - 5 = %d\n", subtract(10, 5));
    printf("10 * 5 = %d\n", multiply(10, 5));
    
    int result = divide(10, 0);
    if (last_error == ERROR_DIVIDE_BY_ZERO) {
        printf("Error: Division by zero!\n");
    }
    
    return 0;
}
```

## The Compilation Process

Let's compile these files separately and examine what happens:

```bash
$ gcc -c mylib.c -o mylib.o
$ gcc -c main.c -o main.o
```

The `-c` flag tells GCC to compile but not link. Let's examine the object files:

```bash
$ file mylib.o main.o
mylib.o: ELF 64-bit relocatable, x86-64, version 1 (SYSV), not stripped
main.o:  ELF 64-bit relocatable, x86-64, version 1 (SYSV), not stripped
```

Notice they're marked as "relocatable" - they're not yet executable files.

## Understanding Object Files

Let's look at the symbols in our object files:

```bash
$ nm mylib.o
0000000000000000 T add
0000000000000000 D last_error
0000000000000020 T multiply
0000000000000010 T subtract
0000000000000030 T divide

$ nm main.o
                 U add
                 U divide
                 U last_error
0000000000000000 T main
                 U multiply
                 U printf
                 U subtract
```

The output shows:
- `T`: Defined text (code) symbols
- `U`: Undefined symbols (to be resolved by the linker)
- `D`: Defined data symbols

## The Linking Process

Now let's link these objects together:

```bash
$ gcc mylib.o main.o -o calculator
```

We can see what the linker did by examining the final executable:

```bash
$ nm calculator
0000000000004010 D last_error
0000000000001129 T add
0000000000001139 T subtract
0000000000001149 T multiply
0000000000001159 T divide
0000000000001169 T main
```

Notice how:
1. All symbols now have final addresses
2. There are no more undefined symbols
3. The addresses are properly spaced to accommodate each function's size

## Linking in Detail

Let's break down what the linker does:

### 1. Symbol Resolution

The linker matches symbol definitions with their uses:

```bash
$ objdump -t main.o | grep add
0000000000000000         *UND*    0000000000000000 add

$ objdump -t mylib.o | grep add
0000000000000000 g     F .text    0000000000000010 add
```

This shows `main.o` has an undefined reference to `add`, which is defined in `mylib.o`.

### 2. Relocation

Let's see the relocations needed in `main.o`:

```bash
$ objdump -r main.o

main.o:     file format elf64-x86-64

RELOCATION RECORDS FOR [.text]:
OFFSET           TYPE              VALUE 
0000000000000019 R_X86_64_PLT32    add-0x0000000000000004
000000000000002f R_X86_64_PLT32    subtract-0x0000000000000004
0000000000000045 R_X86_64_PLT32    multiply-0x0000000000000004
000000000000005b R_X86_64_PLT32    divide-0x0000000000000004


RELOCATION RECORDS FOR [.data]:
OFFSET           TYPE              VALUE 
```

These records tell the linker how to patch the code once final addresses are known.

## Common Linking Issues

### 1. Undefined References

Let's introduce a bug by removing the definition of `multiply`:

```bash
$ # Remove multiply from mylib.c and recompile
$ gcc -c mylib.c -o mylib.o
$ gcc mylib.o main.o -o calculator
/usr/bin/ld: main.o: in function `main':
main.c:(.text+0x45): undefined reference to `multiply'
collect2: error: ld returned 1 exit status
```

### 2. Multiple Definitions

Let's define `last_error` in both files:

```c
// In both mylib.c and main.c
int last_error = 0;
```

```bash
$ gcc mylib.o main.o -o calculator
/usr/bin/ld: main.o:(.data+0x0): multiple definition of `last_error';
mylib.o:(.data+0x0): first defined here
collect2: error: ld returned 1 exit status
```

### 3. Order Matters

The order of object files can matter when linking:

```bash
# This works (libraries after their users)
$ gcc main.o mylib.o -o calculator

# This might fail with some linkers (libraries before their users)
$ gcc mylib.o main.o -o calculator
```

## Static Libraries

Let's turn our calculator into a static library:

```bash
$ ar rcs libcalc.a mylib.o
```

Now we can link against the library:

```bash
$ gcc main.o -L. -lcalc -o calculator
```

Let's examine the library:

```bash
$ ar t libcalc.a
mylib.o

$ nm libcalc.a

mylib.o:
0000000000000000 T add
0000000000000000 D last_error
0000000000000020 T multiply
0000000000000010 T subtract
0000000000000030 T divide
```

## Using the Verbose Linker

We can see exactly what the linker is doing:

```bash
$ gcc -Wl,--verbose main.o -L. -lcalc -o calculator
GNU ld version ...
  attempt to open main.o succeeded
  attempt to open /usr/lib/gcc/...
  attempt to open libcalc.a succeeded
...
```

## Link-Time Optimization

Modern compilers support Link-Time Optimization (LTO):

```bash
$ gcc -flto -c mylib.c -o mylib.o
$ gcc -flto -c main.c -o main.o
$ gcc -flto mylib.o main.o -o calculator_lto
```

Compare sizes:
```bash
$ ls -l calculator calculator_lto
-rwxr-xr-x 1 user user 16704 calculator
-rwxr-xr-x 1 user user 16168 calculator_lto
```

## Controlling Symbol Visibility

We can control which symbols are visible outside our library:

```c
// In mylib.c
__attribute__((visibility("hidden")))
int internal_helper(int x) {
    return x * 2;
}

// This symbol will be visible
int multiply(int a, int b) {
    return internal_helper(a) * b / 2;
}
```

Check the symbols:
```bash
$ gcc -fvisibility=hidden -c mylib.c -o mylib.o
$ nm mylib.o | grep internal_helper
0000000000000000 t internal_helper
```

Note the lowercase 't' indicating a local symbol.

## Advanced Linking Techniques

### 1. Version Scripts

Create a version script (`calc.map`):
```
CALC_1.0 {
    global:
        add;
        subtract;
        multiply;
        divide;
        last_error;
    local:
        *;
};
```

Link with version script:
```bash
$ gcc -Wl,--version-script=calc.map main.o mylib.o -o calculator
```

### 2. Weak Symbols

```c
// Provide a default that can be overridden
__attribute__((weak))
int custom_multiply(int a, int b) {
    return a * b;
}
```

### 3. Common Symbols

```c
// In header
extern int config_value;

// In multiple source files
int config_value = 42;  // Common symbol
```

## Debugging Link Issues

Here are some useful commands for debugging link issues:

```bash
# Show undefined symbols
$ nm -u calculator

# Show all symbols with demangled names
$ nm -C calculator

# Show dependencies
$ ldd calculator

# Show section sizes
$ size calculator
```

## Best Practices

1. **Use Header Guards**
```c
#ifndef MYLIB_H
#define MYLIB_H
// ... header contents ...
#endif
```

2. **Minimize Global Variables**
```c
// Better: use accessor functions
int get_last_error(void);
void set_last_error(int error);
```

3. **Use Static for Internal Functions**
```c
static int internal_helper(int x) {
    // ... implementation ...
}
```

## Conclusion

Understanding the linker is crucial for:
- Building modular C programs
- Debugging link-time errors
- Optimizing binary size and performance
- Managing symbol visibility and versioning

In our next post, "Relocations: The Linker's Patchwork," we'll dive deeper into how the linker patches addresses in your code to make everything work together seamlessly.

For readers interested in more advanced linking topics:
- Symbol versioning will be covered in detail in Post 12
- Dynamic linking will be explored in Post 9
- Custom memory layouts will be discussed in Post 8
