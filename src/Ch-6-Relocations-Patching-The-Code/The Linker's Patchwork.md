# Relocations: The Linker's Patchwork

When you compile C code into object files, the compiler doesn't know the final addresses where functions and variables will reside. Instead, it generates relocation entries - placeholders that the linker later patches with the correct addresses. Today, we'll explore how these relocations work and why they're crucial for creating executable programs.

## Understanding Relocations with a Practical Example

Let's start with a simple example that demonstrates various types of relocations:

```c
// message.h
#ifndef MESSAGE_H
#define MESSAGE_H

extern const char* get_message(void);
extern void print_message(void);
extern int message_count;

#endif

// message.c
#include <stdio.h>
#include "message.h"

const char* messages[] = {
    "Hello, World!",
    "How are you?",
    "Goodbye!"
};

int message_count = 3;

const char* get_message(void) {
    static int index = 0;
    if (index >= message_count) {
        index = 0;
    }
    return messages[index++];
}

void print_message(void) {
    printf("%s\n", get_message());
}

// main.c
#include <stdio.h>
#include "message.h"

int main() {
    printf("There are %d messages:\n", message_count);
    for (int i = 0; i < message_count; i++) {
        print_message();
    }
    return 0;
}
```

## Types of Relocations

Let's compile these files and examine the relocations:

```bash
$ gcc -c message.c -o message.o
$ gcc -c main.c -o main.o
```

Now let's look at the relocations in main.o:

```bash
$ objdump -r main.o

main.o:     file format elf64-x86-64

RELOCATION RECORDS FOR [.text]:
OFFSET           TYPE              VALUE 
0000000000000015 R_X86_64_32S     message_count
000000000000002f R_X86_64_PLT32   print_message-0x4
0000000000000041 R_X86_64_PC32    message_count-0x4

RELOCATION RECORDS FOR [.rela.text]:
OFFSET           TYPE              VALUE 
```

Let's break down the common relocation types:

1. **R_X86_64_32S**: Absolute 32-bit relocation
2. **R_X86_64_PLT32**: Function call via PLT (Procedure Linkage Table)
3. **R_X86_64_PC32**: PC-relative 32-bit relocation

## Understanding Relocation Types in Detail

Let's create examples that trigger each type of relocation:

```c
// relocations.c
extern int global_var;         // Will need absolute relocation
extern void far_func(void);    // Will need PLT relocation
extern int array[];            // Will need PC-relative relocation

int use_relocations(void) {
    int result = global_var;           // R_X86_64_32S
    far_func();                        // R_X86_64_PLT32
    return result + array[global_var]; // R_X86_64_PC32
}
```

Compile and examine:

```bash
$ gcc -c relocations.c -o relocations.o
$ objdump -dr relocations.o

relocations.o:     file format elf64-x86-64

Disassembly of section .text:

0000000000000000 <use_relocations>:
   0:   8b 05 00 00 00 00       mov    0x0(%rip),%eax        # 6 <use_relocations+0x6>
                        2: R_X86_64_PC32        global_var-0x4
   6:   53                      push   %rbx
   7:   89 c3                   mov    %eax,%ebx
   9:   e8 00 00 00 00          call   e <use_relocations+0xe>
                        a: R_X86_64_PLT32       far_func-0x4
   e:   48 63 c3                movslq %ebx,%rax
  11:   8b 04 85 00 00 00 00    mov    0x0(,%rax,4),%eax
                        14: R_X86_64_32S        array
  18:   01 d8                   add    %ebx,%eax
  1a:   5b                      pop    %rbx
  1b:   c3                      ret    
```

## How Relocations Are Applied

Let's see how the linker patches these relocations. First, we'll create a complete example:

```c
// Define our external symbols
int global_var = 42;
int array[] = {1, 2, 3, 4, 5};
void far_func(void) {
    // Some implementation
}
```

Now compile and link:

```bash
$ gcc -o program *.c
```

We can see the final addresses using:

```bash
$ nm program | grep -E 'global_var|array|far_func'
0000000000004020 D global_var
0000000000004040 D array
0000000000001129 T far_func
```

## Relocation Calculations

Let's see how each type of relocation is calculated:

### 1. Absolute Relocations (R_X86_64_32S)

```c
// Before linking:
mov    $0x0,%eax    // Placeholder value

// After linking:
mov    $0x4020,%eax // Actual address of global_var
```

### 2. PC-Relative Relocations (R_X86_64_PC32)

```c
// Before linking:
lea    0x0(%rip),%rax    // Placeholder offset

// After linking:
lea    -0x2e(%rip),%rax  // Calculated offset to target
```

### 3. PLT Relocations (R_X86_64_PLT32)

```c
// Before linking:
call   0x0                // Placeholder offset

// After linking:
call   0x1020             // Offset to PLT entry
```

## Debugging Relocation Issues

Common relocation problems and how to debug them:

### 1. Undefined References

```bash
$ gcc -c main.c -o main.o
$ gcc main.o -o program
/usr/bin/ld: main.o: in function `main':
main.c:(.text+0x15): undefined reference to `message_count'
```

Fix by ensuring all symbols are defined:
```bash
$ nm main.o | grep ' U ' # List undefined symbols
$ nm message.o | grep ' D ' # List defined data symbols
```

### 2. Position-Independent Code Issues

```c
// Compile with -fPIC
$ gcc -c -fPIC message.c -o message.o
$ objdump -r message.o # Notice different relocation types
```

### 3. Text Relocations

```bash
$ gcc -shared -o libmessage.so message.o
/usr/bin/ld: message.o: relocation R_X86_64_32S against `.data' can not be used when making a shared object; recompile with -fPIC
```

## Advanced Relocation Topics

### 1. Copy Relocations

These occur when a shared library exports a data symbol:

```c
// In shared library
extern int shared_data;

// In main program
int shared_data;  // Will trigger copy relocation
```

### 2. GOT Relocations

Global Offset Table entries for position-independent code:

```c
extern int global_var;

int get_global(void) {
    return global_var;  // Will use GOT in PIC
}
```

### 3. IFUNC Relocations

GNU indirect functions for runtime symbol resolution:

```c
#include <stdlib.h>

typedef int (*func_type)(int);

static int slow_impl(int x) { return x * x; }
static int fast_impl(int x) { return x << 1; }

static func_type resolve_func(void) {
    if (getenv("FAST_MODE"))
        return fast_impl;
    return slow_impl;
}

__attribute__((ifunc("resolve_func")))
int compute(int x);
```

## Performance Implications

Different relocation types have different performance characteristics:

1. **Absolute Relocations**
   - Fastest at runtime
   - Requires load-time fixups

2. **PC-Relative Relocations**
   - Good performance
   - Position-independent
   - Smaller code size

3. **PLT Relocations**
   - Additional indirection
   - Enables lazy binding
   - Slightly slower

## Best Practices

1. **Use -fPIC When Needed**
```bash
$ gcc -fPIC -c library.c # For shared libraries
```

2. **Minimize Global Variables**
```c
// Prefer:
static const int LOCAL_CONSTANT = 42;

// Over:
const int GLOBAL_CONSTANT = 42;
```

3. **Group Related Symbols**
```c
// Better locality:
struct Config {
    int value1;
    int value2;
} config;

// Instead of:
int config_value1;
int config_value2;
```

## Conclusion

Understanding relocations is crucial for:
- Debugging linking errors
- Writing efficient position-independent code
- Creating shared libraries
- Optimizing program load time

In our next post, "Symbols: The Linker's Address Book," we'll explore how symbols work in conjunction with relocations to create the final executable.

For readers interested in more advanced topics:
- Dynamic linking and the PLT will be covered in detail in Post 9
- Symbol versioning and visibility will be explored in Post 12
- Advanced linking techniques will be discussed in Post 13
