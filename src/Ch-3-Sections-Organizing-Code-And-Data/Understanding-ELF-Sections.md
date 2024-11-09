# Where Your C Code Lives: Understanding ELF Sections

When you compile a C program, your code and data don't just get thrown together randomly in the resulting executable. Instead, they're carefully organized into different sections, each with its own purpose and characteristics. In this post, we'll dive deep into the most important ELF sections and see exactly where different parts of your C code end up.

## The Four Primary Sections

Every ELF executable typically contains four main sections that hold different types of program content:

1. `.text`: Contains executable code
2. `.rodata`: Holds read-only data (constants)
3. `.data`: Contains initialized global and static variables
4. `.bss`: Reserved for uninitialized global and static variables

Let's explore each of these sections in detail with practical examples.

## The .text Section: Where Your Code Lives

The `.text` section contains the actual machine code instructions that your program executes. Let's look at a simple example:

```c
// example1.c
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(5, 3);
    return result;
}
```

After compiling this code (`gcc -o example1 example1.c`), we can examine the `.text` section:

```bash
$ objdump -d -j .text example1

example1:     file format elf64-x86-64

Disassembly of section .text:

0000000000001129 <add>:
    1129:    55                      push   %rbp
    112a:    48 89 e5                mov    %rsp,%rbp
    112d:    89 7d fc                mov    %edi,-0x4(%rbp)
    1130:    89 75 f8                mov    %esi,-0x8(%rbp)
    1133:    8b 55 fc                mov    -0x4(%rbp),%edx
    1136:    8b 45 f8                mov    -0x8(%rbp),%eax
    1139:    01 d0                   add    %edx,%eax
    113b:    5d                      pop    %rbp
    113c:    c3                      ret    

000000000000113d <main>:
    113d:    55                      push   %rbp
    113e:    48 89 e5                mov    %rsp,%rbp
    1141:    48 83 ec 10             sub    $0x10,%rsp
    1145:    be 03 00 00 00          mov    $0x3,%esi
    114a:    bf 05 00 00 00          mov    $0x5,%edi
    114f:    e8 d5 ff ff ff          call   1129 <add>
    1154:    89 45 fc                mov    %eax,-0x4(%rbp)
    1157:    8b 45 fc                mov    -0x4(%rbp),%eax
    115a:    c9                      leave  
    115b:    c3                      ret    
```

This output shows the actual machine code instructions for both our `add` and `main` functions. Each function's code is stored sequentially in the `.text` section, with the addresses shown on the left.

## The .rodata Section: Constants and String Literals

The `.rodata` (read-only data) section stores constant values that shouldn't be modified during program execution. Let's look at an example:

```c
// example2.c
const char* message = "Hello, World!";
const int magic_number = 42;

int main() {
    return magic_number;
}
```

We can examine the `.rodata` section:

```bash
$ objdump -s -j .rodata example2

example2:     file format elf64-x86-64

Contents of section .rodata:
 2000 48656c6c 6f2c2057 6f726c64 2100      Hello, World!.
```

Note how our string constant "Hello, World!" is stored in the `.rodata` section. The magic number is typically handled by the compiler as an immediate value in the instructions.

## The .data Section: Initialized Global Variables

The `.data` section contains global and static variables that are initialized with non-zero values. Here's an example:

```c
// example3.c
int global_counter = 100;
static int static_value = 42;

int main() {
    global_counter++;
    return static_value;
}
```

Let's look at the `.data` section:

```bash
$ objdump -s -j .data example3

example3:     file format elf64-x86-64

Contents of section .data:
 4000 64000000 2a000000                    d...*...
```

Here we can see our initialized values: 100 (0x64) and 42 (0x2a) stored in little-endian format.

## The .bss Section: Uninitialized Variables

The `.bss` section is unique because it doesn't actually take up space in the executable file. It only specifies how much space should be allocated for uninitialized global and static variables when the program loads. Let's see an example:

```c
// example4.c
int uninitialized_array[1000];
static char buffer[4096];

int main() {
    uninitialized_array[0] = 42;
    buffer[0] = 'A';
    return 0;
}
```

We can examine the `.bss` section's size:

```bash
$ readelf -S example4 | grep bss
  [24] .bss              NOBITS           0000000000004020 002020 001010 00  WA  0   0 32
```

Note that the size is 0x1010 bytes (4112 in decimal), which accounts for our array (4000 bytes) and buffer (4096 bytes), aligned appropriately.

## Section Permissions and Memory Layout

Each section has specific permissions that determine how it can be accessed at runtime:

- `.text`: Read + Execute
- `.rodata`: Read-only
- `.data`: Read + Write
- `.bss`: Read + Write

We can verify these permissions:

```bash
$ readelf -S example1 | grep -E "text|rodata|data|bss"
  [14] .text             PROGBITS         0000000000001129 001129 000033 00  AX  0   0 16
  [15] .rodata           PROGBITS         0000000000002000 002000 000010 00   A  0   0  4
  [23] .data             PROGBITS         0000000000004000 003000 000010 00  WA  0   0  8
  [24] .bss              NOBITS           0000000000004010 003010 001000 00  WA  0   0 32
```

The flags column shows:
- `A` = Allocatable
- `X` = Executable
- `W` = Writable

## Advanced Section Concepts

While we've covered the main sections here, ELF files contain many other specialized sections for different purposes:

- `.init` and `.fini`: Constructor and destructor code
- `.plt` and `.got`: Supporting dynamic linking (covered in detail in Post 9)
- `.debug`: Debugging information
- `.eh_frame`: Exception handling data

These sections serve specific purposes and become important when dealing with more complex programs, especially those using dynamic linking or requiring debug information.

## Practical Tips for Working with Sections

1. Use `strip` to remove unnecessary sections:
```bash
$ strip --strip-all example1
```

2. Add custom sections using GCC attributes:
```c
__attribute__((section(".mysection"))) int special_var = 42;
```

3. Examine section sizes to optimize binary size:
```bash
$ size example1
   text    data     bss     dec     hex filename
   1825     600      16    2441     989 example1
```

## Understanding Section Impact on Program Behavior

The way your code is organized into sections can affect:

1. Memory usage: The `.bss` section doesn't take up file space but requires memory at runtime
2. Security: Section permissions help prevent code injection and data corruption
3. Performance: Code and data alignment within sections can impact cache efficiency
4. Load time: Fewer sections generally mean faster program loading

## Conclusion

Understanding ELF sections is crucial for:
- Debugging memory-related issues
- Optimizing program size and performance
- Implementing security measures
- Working with embedded systems where memory layout matters

In our next post, "Before main(): The Secret Life of Global Variables in C," we'll explore how these sections come into play during program initialization, particularly focusing on how global variables are set up before your program's main function begins execution.

For readers interested in more specific details about section permissions and memory mapping, these topics will be covered in depth in Post 8, "Customizing the Layout: Introduction to Linker Scripts."
