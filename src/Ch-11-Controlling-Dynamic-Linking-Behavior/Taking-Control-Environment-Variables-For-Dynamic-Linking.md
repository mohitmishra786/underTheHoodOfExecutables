# Chapter 11: Taking Control: Environment Variables for Dynamic Linking

## Introduction

In our previous chapter, we explored lazy loading and its role in dynamic linking. Now, let's dive into the powerful environment variables that give us fine-grained control over the dynamic linking process. These variables allow us to modify linking behavior, debug issues, and even intercept library calls—all without changing our source code.

## Core Dynamic Linking Environment Variables

### LD_LIBRARY_PATH

This is perhaps the most commonly used environment variable for dynamic linking. It specifies additional directories where the dynamic linker should look for shared libraries.

```bash
# Basic usage
$ export LD_LIBRARY_PATH=/path/to/my/libs:/another/path
$ ./myprogram

# Temporary usage for a single command
$ LD_LIBRARY_PATH=/path/to/my/libs ./myprogram
```

Let's create a practical example to demonstrate its use:

```c
// mylib.c
#include <stdio.h>

void print_message(void) {
    printf("This is version 1.0 of the library\n");
}

// Compile as shared library
// gcc -shared -fPIC -o libmylib.so mylib.c
```

```c
// main.c
extern void print_message(void);

int main() {
    print_message();
    return 0;
}

// Compile with:
// gcc -o main main.c -L. -lmylib
```

Now we can control which version of the library is loaded:

```bash
$ mkdir v1 v2
$ cp libmylib.so v1/
$ # Create a different version in v2
$ LD_LIBRARY_PATH=./v1 ./main
This is version 1.0 of the library
$ LD_LIBRARY_PATH=./v2 ./main
This is version 2.0 of the library
```

### LD_BIND_NOW

As discussed in Chapter 10, this variable forces immediate binding of all symbols at program startup, disabling lazy binding:

```bash
# Normal lazy loading behavior
$ time ./myprogram
Real time: 0.001s

# Force immediate binding
$ time LD_BIND_NOW=1 ./myprogram
Real time: 0.003s
```

Let's create a program to demonstrate the impact:

```c
// binding_test.c
#include <stdio.h>
#include <time.h>
#include <dlfcn.h>

void (*func_ptr)(void);

int main() {
    clock_t start, end;
    double cpu_time_used;

    // Measure library loading time
    start = clock();
    void* handle = dlopen("libheavy.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Library loading time: %f seconds\n", cpu_time_used);

    // Measure function resolution time
    start = clock();
    func_ptr = dlsym(handle, "heavy_function");
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Symbol resolution time: %f seconds\n", cpu_time_used);

    return 0;
}
```

### LD_DEBUG

This powerful variable enables detailed debugging output for the dynamic linker. It accepts multiple options:

```bash
# Available debug options
$ LD_DEBUG=help ./myprogram

Common useful options:
- bindings: Display information about symbol binding
- libs: Show library search paths and loading
- versions: Print version dependencies
- statistics: Show relocation statistics
- all: Enable all debug options
```

Let's create a program that demonstrates various LD_DEBUG options:

```c
// debug_demo.c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern double calculate_something(double x);

int main() {
    double result = calculate_something(42.0);
    printf("Result: %f\n", result);
    return 0;
}
```

```c
// libcalc.c
#include <math.h>

double calculate_something(double x) {
    return sin(x) * cos(x) * exp(x/100.0);
}
```

Compile and observe the debug output:

```bash
$ gcc -shared -fPIC -o libcalc.so libcalc.c -lm
$ gcc -o debug_demo debug_demo.c -L. -lcalc -Wl,-rpath,.

# Observe library loading
$ LD_DEBUG=libs ./debug_demo

# Watch symbol binding
$ LD_DEBUG=bindings ./debug_demo

# See everything
$ LD_DEBUG=all ./debug_demo 2>&1 | less
```

### LD_PRELOAD

This powerful (and potentially dangerous) variable allows us to intercept library calls by preloading our own libraries. It's commonly used for:
- Debugging
- Profiling
- Security monitoring
- Function interception

Let's create a practical example of function interception:

```c
// malloc_tracker.c
#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>

// Function pointer for the real malloc
static void* (*real_malloc)(size_t) = NULL;

// Our malloc wrapper
void* malloc(size_t size) {
    // Initialize real_malloc if needed
    if (!real_malloc) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
        if (!real_malloc) {
            fprintf(stderr, "Error getting malloc symbol: %s\n", dlerror());
            exit(1);
        }
    }

    // Call the real malloc
    void* ptr = real_malloc(size);

    // Log the allocation
    fprintf(stderr, "malloc(%zu) = %p\n", size, ptr);

    return ptr;
}

// Compile with:
// gcc -shared -fPIC -o malloc_tracker.so malloc_tracker.c -ldl
```

Now we can track malloc calls in any program:

```bash
$ LD_PRELOAD=./malloc_tracker.so ./myprogram
malloc(1024) = 0x55a7b4567000
malloc(512) = 0x55a7b4567400
...
```

## Advanced Environment Variable Usage

### LD_SHOW_AUXV

Shows auxiliary vector information passed to the program:

```bash
$ LD_SHOW_AUXV=1 ./myprogram
AT_SYSINFO_EHDR: 0x7ffff7fd9000
AT_HWCAP:        bfebfbff
AT_PAGESZ:       4096
AT_CLKTCK:       100
...
```

### LD_DYNAMIC_WEAK

Controls how weak symbols are handled during dynamic linking:

```c
// weak_symbol_demo.c
#include <stdio.h>

__attribute__((weak)) void optional_function(void) {
    printf("Default implementation\n");
}

int main() {
    if (optional_function) {
        optional_function();
    } else {
        printf("Function not available\n");
    }
    return 0;
}
```

### LD_TRACE_LOADED_OBJECTS

Similar to ldd, shows shared library dependencies:

```bash
$ LD_TRACE_LOADED_OBJECTS=1 ./myprogram
        linux-vdso.so.1 =>  (0x00007ffff7ffa000)
        libmylib.so => ./libmylib.so (0x00007ffff7fb4000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007ffff7bed000)
```

## Security Considerations

### Risks and Mitigations

1. LD_PRELOAD Risks:
```c
// security_check.c
#include <stdlib.h>
#include <unistd.h>

int main() {
    if (getuid() == 0) {  // Running as root
        unsetenv("LD_PRELOAD");  // Clear LD_PRELOAD for security
    }
    // Continue with program
    return 0;
}
```

2. Library Search Path Security:
```c
// secure_lib_load.c
#include <stdlib.h>
#include <string.h>

void secure_library_init(void) {
    // Clear potentially dangerous environment variables
    unsetenv("LD_LIBRARY_PATH");
    unsetenv("LD_PRELOAD");
    
    // Set up secure library path
    setenv("LD_LIBRARY_PATH", "/opt/secure/lib", 1);
}
```

## Debugging Techniques Using Environment Variables

### Creating a Debug Helper

```c
// debug_helper.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setup_debug_env(const char* debug_options) {
    char debug_cmd[256];
    snprintf(debug_cmd, sizeof(debug_cmd), 
             "LD_DEBUG=%s LD_DEBUG_OUTPUT=/tmp/debug.log", 
             debug_options);
    system(debug_cmd);
}

// Usage example
int main() {
    setup_debug_env("libs:bindings");
    // Your program logic here
    return 0;
}
```

### Automated Testing Script

```bash
#!/bin/bash
# test_dynamic_linking.sh

TEST_LIBS="lib1.so lib2.so lib3.so"
DEBUG_OPTIONS="libs bindings versions statistics"

for lib in $TEST_LIBS; do
    echo "Testing $lib..."
    for opt in $DEBUG_OPTIONS; do
        echo "Debug option: $opt"
        LD_DEBUG=$opt LD_LIBRARY_PATH=./test_libs ./test_program 2>debug_${lib}_${opt}.log
    done
done
```

## Performance Optimization Using Environment Variables

### Measuring Impact

```c
// perf_test.c
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

double measure_startup(const char* env_var) {
    clock_t start = clock();
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s ./myprogram >/dev/null 2>&1", 
             env_var ? env_var : "");
    system(cmd);
    
    clock_t end = clock();
    return ((double) (end - start)) / CLOCKS_PER_SEC;
}

int main() {
    printf("Normal startup: %f seconds\n", 
           measure_startup(NULL));
    printf("With LD_BIND_NOW: %f seconds\n", 
           measure_startup("LD_BIND_NOW=1"));
    return 0;
}
```

## References and Further Reading

1. Dynamic Linker Manual: man ld.so
2. Security Implications of Dynamic Linking: https://www.cs.columbia.edu/~vpk/research/dynamic-loading/
3. Linux Foundation Documentation on Dynamic Linking: https://www.linuxfoundation.org/
4. Advanced Linux Programming, Chapter 3: https://mentorembedded.github.io/advancedlinuxprogramming/
5. The GNU C Library manual section on Dynamic Linking: https://www.gnu.org/software/libc/manual/

*Note: For more information about lazy loading and its implementation details, refer to Chapter 10. For an in-depth exploration of weak symbols and their behavior, see Chapter 12.*
