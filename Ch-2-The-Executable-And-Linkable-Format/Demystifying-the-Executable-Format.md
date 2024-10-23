# ELF: Demystifying the Executable Format

In Chapter 1, we discovered that our simple "Hello, World!" program resulted in a surprisingly large executable file. Now, let's dive deep into the format that makes this all possible: the Executable and Linkable Format (ELF). This chapter will explore the structure, purpose, and intricacies of ELF files, the standard binary format for executables on Linux systems.

## The Anatomy of an ELF File

An ELF file is like a carefully organized container that holds various components of our program. Let's create a simple program to examine throughout this chapter:

```c
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
```

Let's compile this and begin our exploration:

```bash
$ gcc -g -o example example.c
```

### ELF File Structure Overview

An ELF file consists of four main parts:
1. ELF Header
2. Program Header Table
3. Sections
4. Section Header Table

Here's a visual representation:

```
+-------------------+
|    ELF Header     |
+-------------------+
| Program Headers   |
+-------------------+
|                   |
|     Sections      |
|    (.text, etc)   |
|                   |
+-------------------+
| Section Headers   |
+-------------------+
```

## The ELF Header: The File's Identity Card

Let's examine the ELF header in detail:

```bash
$ readelf -h example
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                           UNIX - System V
  ABI Version:                       0
  Type:                             DYN (Shared object file)
  Machine:                          Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x1060
  Start of program headers:          64 (bytes into file)
  Start of section headers:          14816 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         13
  Size of section headers:           64 (bytes)
  Number of section headers:         31
  Section header string table index: 30
```

Let's break down these fields:

### 1. Magic Number
```
Magic: 7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
```
- First four bytes are always: 0x7F, 'E', 'L', 'F'
- Following bytes specify:
  - File class (32/64 bit)
  - Data encoding
  - ELF version
  - OS ABI
  - ABI version
  - Padding

### 2. File Type
```
Type: DYN (Shared object file)
```
Common types include:
- `EXEC`: Executable file
- `DYN`: Shared object file (including position-independent executables)
- `REL`: Relocatable file
- `CORE`: Core dump file

### 3. Machine Architecture
```
Machine: Advanced Micro Devices X86-64
```
Specifies the target architecture. Other common values:
- `EM_386`: x86
- `EM_ARM`: ARM
- `EM_RISCV`: RISC-V

### 4. Entry Point
```
Entry point address: 0x1060
```
The memory address where program execution begins. This typically points to the `_start` function, not `main` (we'll explore why in Chapter 4).

## Program Headers: Loading Instructions

Program headers tell the system how to create the process image. Let's examine them:

```bash
$ readelf -l example

Elf file type is DYN (Position-Independent Executable file)
Entry point 0x1060
There are 13 program headers, starting at offset 64

Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  PHDR           0x0000000000000040 0x0000000000000040 0x0000000000000040
                 0x00000000000002d8 0x00000000000002d8  R      0x8
  INTERP         0x0000000000000318 0x0000000000000318 0x0000000000000318
                 0x000000000000001c 0x000000000000001c  R      0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
  LOAD           0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000898 0x0000000000000898  R      0x1000
  LOAD           0x0000000000001000 0x0000000000001000 0x0000000000001000
                 0x0000000000000235 0x0000000000000235  R E    0x1000
  LOAD           0x0000000000002000 0x0000000000002000 0x0000000000002000
                 0x00000000000001a4 0x00000000000001a4  R      0x1000
  LOAD           0x0000000000002db8 0x0000000000003db8 0x0000000000003db8
                 0x0000000000000258 0x0000000000000260  RW     0x1000
```

Key program header types:

### 1. PHDR
Points to the program header table itself.

### 2. INTERP
Specifies the dynamic linker/interpreter:
```bash
$ readelf -p .interp example
String dump of section '.interp':
  [     0]  /lib64/ld-linux-x86-64.so.2
```

### 3. LOAD
Describes segments to be loaded into memory:
- Read-only segments (code and constants)
- Read-execute segments (code)
- Read-write segments (data)

### 4. DYNAMIC
Contains dynamic linking information:
```bash
$ readelf -d example
Dynamic section at offset 0x2dc8 contains 27 entries:
  Tag        Type                         Name/Value
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
 0x000000000000000c (INIT)               0x1000
 0x000000000000000d (FINI)               0x11f8
 [...]
```

## Sections: Where Program Components Live

Let's get a comprehensive view of the sections:

```bash
$ readelf -S example
There are 31 section headers, starting at offset 0x39f0:

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .interp           PROGBITS         0000000000000318  00000318
       000000000000001c  0000000000000000   A       0     0     1
  [ 2] .note.gnu.property NOTE             0000000000000338  00000338
       0000000000000030  0000000000000000   A       0     0     8
  [...]
```

Important sections include:

### 1. Code Sections
- `.text`: Executable code
- `.init`: Initialization code
- `.fini`: Finalization code
- `.plt`: Procedure Linkage Table (for dynamic linking)

### 2. Data Sections
- `.data`: Initialized data
- `.rodata`: Read-only data
- `.bss`: Uninitialized data
- `.got`: Global Offset Table

### 3. Symbol and Relocation Sections
- `.symtab`: Symbol table
- `.strtab`: String table
- `.rela.text`: Relocations for .text
- `.rela.data`: Relocations for .data

## Examining Section Contents

Let's look at some specific sections:

### 1. Code in .text
```bash
$ objdump -d -j .text example
...
0000000000001169 <print_message>:
    1169:       55                      push   %rbp
    116a:       48 89 e5                mov    %rsp,%rbp
    116d:       48 8d 05 94 0e 00 00    lea    0xe94(%rip),%rax
    1174:       48 89 c7                mov    %rax,%rdi
    1177:       e8 e4 fe ff ff          call   1060 <puts@plt>
    117c:       90                      nop
    117d:       5d                      pop    %rbp
    117e:       c3                      ret
...
```

### 2. Read-only Data in .rodata
```bash
$ objdump -s -j .rodata example
Contents of section .rodata:
 2000 01000200 48656c6c 6f206672 6f6d2045  ....Hello from E
 2010 4c462100                             LF!.
```

### 3. Initialized Data in .data
```bash
$ objdump -s -j .data example
Contents of section .data:
 4000 2a000000                             *...
```

## Symbol Tables: The Program's Directory

Symbol tables map names to addresses:

```bash
$ readelf -s example
Symbol table '.dynsym' contains 7 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND puts@GLIBC_2.2.5 (2)
     2: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND __gmon_start__
     3: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND _ITM_deregisterT[...]
     4: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND _ITM_registerTMC[...]
     5: 0000000000000000     0 FUNC    WEAK   DEFAULT  UND [...]@GLIBC_2.2.5 (2)
     6: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND __cxa_finalize@[...]
```

## Working with ELF Files

### 1. Creating Different Types of ELF Files

#### Relocatable Object File
```bash
$ gcc -c -o example.o example.c
$ file example.o
example.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
```

#### Shared Library
```bash
$ gcc -shared -fPIC -o libexample.so example.c
$ file libexample.so
libexample.so: ELF 64-bit LSB shared object, x86-64, version 1 (SYSV)
```

#### Static Executable
```bash
$ gcc -static -o example_static example.c
$ file example_static
example_static: ELF 64-bit LSB executable, x86-64, version 1 (GNU/Linux)
```

### 2. Manipulating ELF Files

#### Adding Sections
```bash
$ objcopy --add-section .mydata=myfile.txt example example_modified
```

#### Stripping Debug Information
```bash
$ strip --strip-debug example
```

#### Extracting Sections
```bash
$ objcopy -O binary --only-section=.text example text.bin
```

## ELF in Different Contexts

### 1. Relocatable Files (.o)
- Contain code and data suitable for linking
- Have relocations that specify how to modify section contents
- Include symbol table for external references

### 2. Executable Files
- Contain directly executable code and data
- Have program headers describing how to create process image
- Specify interpreter for dynamic linking

### 3. Shared Objects (.so)
- Can be linked at runtime
- Position-independent code
- Can be shared by multiple processes

## Practical Applications

### 1. Debugging
```bash
$ gdb example
(gdb) info files
Symbols from "/path/to/example".
Local exec file:
        `/path/to/example', file type elf64-x86-64.
        Entry point: 0x1060
        0x0000000000000318 - 0x0000000000000334 is .interp
        [...]
```

### 2. Binary Analysis
```bash
$ nm example
0000000000004010 B __bss_start
0000000000004010 b completed.8061
                 w __cxa_finalize@@GLIBC_2.2.5
0000000000004000 D __data_start
[...]
```

### 3. Runtime Analysis
```bash
$ strace ./example
execve("./example", ["./example"], 0x7ffc9487d7e0 /* 66 vars */) = 0
brk(NULL)                               = 0x557a5a43c000
arch_prctl(ARCH_SET_FS, 0x557a5a43b540) = 0
[...]
```

## Security Implications

### 1. ASLR (Address Space Layout Randomization)
```bash
$ cat /proc/sys/kernel/randomize_va_space
2
$ ./example
$ cat /proc/$(pgrep example)/maps
00400000-00401000 r-xp 00000000 08:01 1048578    /path/to/example
[...]
```

### 2. Stack Protection
```bash
$ readelf -s example | grep -i stack
     8: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND __stack_chk_fail@GLIBC_2.4
```

### 3. Read-only Relocations
```bash
$ readelf -d example | grep RELRO
 0x000000000000001e (FLAGS)              BIND_NOW RTLD_GLOBAL
```

## Conclusion

Understanding the ELF format is crucial for:
- Debugging complex problems
- Understanding program loading and execution
- Implementing security measures
- Optimizing program size and performance

In the next chapter, we'll dive deeper into ELF sections and explore how different types of code and data are organized within them.

## Further Reading

1. Official References:
   - System V ABI specification
   - Tool Interface Standard (TIS) ELF specification

2. Command Documentation:
   - `man elf`
   - `man objdump`
   - `man readelf`

3. Online Resources:
   - Linux Foundation documentation
   - ELF Tool Chain project
