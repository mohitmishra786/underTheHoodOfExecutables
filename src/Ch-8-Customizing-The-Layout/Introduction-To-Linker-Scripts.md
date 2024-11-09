# Chapter 8: Customizing the Layout - Introduction to Linker Scripts

## Introduction to Linker Scripts

Linker scripts are powerful tools that give you precise control over how your program is organized in memory. While most developers can rely on the default linker script provided by their toolchain, understanding linker scripts becomes crucial when:
- Developing embedded systems
- Creating bootloaders
- Optimizing memory usage
- Implementing custom memory layouts
- Working with specialized hardware

## Basic Concepts

### Memory and Sections

Let's start with a basic linker script that demonstrates the fundamental concepts:

```
/* basic.ld */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K
    CCRAM (rwx) : ORIGIN = 0x10000000, LENGTH = 64K
}

SECTIONS
{
    .text : {
        *(.text*)
        *(.rodata*)
    } > FLASH

    .data : {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > SRAM AT > FLASH

    .bss : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > SRAM
}
```

Let's break down each component:

### 1. Memory Regions

```
MEMORY
{
    region_name (attributes) : ORIGIN = address, LENGTH = size
}
```

Attributes can be:
- `r`: Read
- `w`: Write
- `x`: Execute
- `a`: Allocatable
- `i`: Initialized

Example with different memory types:
```
MEMORY
{
    FLASH (rx)    : ORIGIN = 0x08000000, LENGTH = 512K  /* Program code */
    SRAM (rwx)    : ORIGIN = 0x20000000, LENGTH = 128K  /* Main RAM */
    CCRAM (rwx)   : ORIGIN = 0x10000000, LENGTH = 64K   /* Core-coupled RAM */
    BACKUP (rw)   : ORIGIN = 0x40024000, LENGTH = 4K    /* Backup RAM */
    OTP (r)       : ORIGIN = 0x1FFF7800, LENGTH = 512   /* One-time programmable */
}
```

## Practical Implementation

Let's create a complete example using a custom linker script:

```c
// startup.c
#include <stdint.h>

// These symbols are defined in our linker script
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sidata;
extern uint32_t _sbss;
extern uint32_t _ebss;

void Reset_Handler(void) {
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    // Copy initialized data from flash to SRAM
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    // Zero out BSS section
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    // Call main
    main();
}

// Vector table
__attribute__((section(".vector_table")))
const uint32_t vector_table[] = {
    (uint32_t)&_estack,         // Initial stack pointer
    (uint32_t)&Reset_Handler,   // Reset handler
    // Add other vectors as needed
};
```

Corresponding linker script:
```
/* custom_layout.ld */
MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM (rwx)      : ORIGIN = 0x20000000, LENGTH = 128K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .vector_table : {
        . = ALIGN(4);
        KEEP(*(.vector_table))
        . = ALIGN(4);
    } > FLASH

    .text : {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    _sidata = LOADADDR(.data);

    .data : {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    .bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    /* Stack and heap configurations */
    .stack : {
        . = ALIGN(8);
        . = . + 0x2000;    /* 8KB stack */
        . = ALIGN(8);
        _estack = .;
    } > SRAM
}
```

## Advanced Linker Script Features

### 1. Section Alignment and Padding

```
SECTIONS
{
    .text : {
        . = ALIGN(4);        /* Align to 4-byte boundary */
        *(.text*)
        . = ALIGN(4);
        
        /* Add padding to align next section on cache line */
        . = ALIGN(32);       /* Assuming 32-byte cache lines */
    } > FLASH

    .rodata : {
        . = ALIGN(4);
        *(.rodata*)
        /* Force section to specific size */
        . = ALIGN(4);
        . = . + 1024;        /* Add 1KB padding */
    } > FLASH
}
```

### 2. Memory Region Overlays

```
MEMORY
{
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

SECTIONS
{
    .overlay1 : {
        *(.overlay1*)
    } > RAM

    .overlay2 : {
        *(.overlay2*)
    } > RAM AT > RAM

    .overlay3 : {
        *(.overlay3*)
    } > RAM AT > RAM
}

OVERLAY : {
    .overlay1 { *(.overlay1*) }
    .overlay2 { *(.overlay2*) }
    .overlay3 { *(.overlay3*) }
} > RAM
```

### 3. Custom Section Ordering and Grouping

```
SECTIONS
{
    .text : {
        /* Core functions first */
        *(.text.startup*)
        *(.text.Reset_Handler)
        *(.text.interrupt_handlers)
        
        /* Then regular code */
        *(.text*)
        
        /* Critical functions last */
        *(.text.critical*)
    } > FLASH

    .rodata : {
        /* Constants used by startup code */
        *(.rodata.startup*)
        /* All other constants */
        *(.rodata*)
    } > FLASH
}
```

### 4. Memory Protection Unit (MPU) Configuration

```
SECTIONS
{
    /* MPU requires regions to be properly aligned */
    .text : ALIGN(0x20) {
        _stext = .;
        *(.text*)
        . = ALIGN(0x20);
        _etext = .;
    } > FLASH

    /* Read-only data in separate MPU region */
    .rodata : ALIGN(0x20) {
        _srodata = .;
        *(.rodata*)
        . = ALIGN(0x20);
        _erodata = .;
    } > FLASH

    /* Writeable data in separate MPU region */
    .data : ALIGN(0x20) {
        _sdata = .;
        *(.data*)
        . = ALIGN(0x20);
        _edata = .;
    } > SRAM AT > FLASH
}
```

## Debugging Linker Scripts

### 1. Memory Map Generation

```bash
$ arm-none-eabi-ld -Map=output.map -T custom_layout.ld input.o
```

Example map file content:
```
Memory Configuration

Name             Origin             Length             Attributes
FLASH            0x08000000         0x00080000         xr
SRAM             0x20000000         0x00020000         xrw

Linker script and memory map

.vector_table    0x08000000       0x100
                0x08000000        0x40 startup.o
                ...

.text           0x08000100      0x1234
                0x08000100       0x450 main.o
                0x08000550       0x7e4 lib.o
```

### 2. Section Analysis

```bash
$ arm-none-eabi-objdump -h program.elf
```

Output:
```
program.elf:     file format elf32-littlearm

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .vector_table 00000100  08000000  08000000  00010000  2**2
  1 .text         00001234  08000100  08000100  00010100  2**2
  2 .data         00000100  20000000  08001334  00020000  2**2
  3 .bss          00000200  20000100  20000100  00020100  2**2
```

### 3. Symbol Location Verification

```bash
$ arm-none-eabi-nm program.elf
```

## Real-World Examples

### 1. Bootloader Layout

```
MEMORY
{
    BOOTROM (rx)  : ORIGIN = 0x08000000, LENGTH = 16K
    APPROM (rx)   : ORIGIN = 0x08004000, LENGTH = 496K
    SRAM (rwx)    : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    .bootloader : {
        *(.bootloader*)
        *(.boot_vector*)
    } > BOOTROM

    .app_vector_table : {
        *(.app_vectors*)
    } > APPROM

    .text : {
        *(.text*)
    } > APPROM

    /* ... rest of sections ... */
}
```

### 2. DMA-Optimized Layout

```
SECTIONS
{
    /* DMA buffers aligned to cache line size */
    .dma_buffers : ALIGN(32) {
        *(.dma_tx_buffer*)
        . = ALIGN(32);
        *(.dma_rx_buffer*)
    } > SRAM

    /* Regular data after DMA buffers */
    .data : {
        *(.data*)
    } > SRAM
}
```

### 3. Dual-Bank Flash Layout

```
MEMORY
{
    FLASH_BANK0 (rx) : ORIGIN = 0x08000000, LENGTH = 256K
    FLASH_BANK1 (rx) : ORIGIN = 0x08040000, LENGTH = 256K
    SRAM (rwx)       : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    .bank0_code : {
        *(.bank0_text*)
    } > FLASH_BANK0

    .bank1_code : {
        *(.bank1_text*)
    } > FLASH_BANK1
}
```

## Best Practices and Optimization Tips

1. **Cache Alignment**
```
.text : ALIGN(32) {  /* Align to cache line size */
    *(.text*)
}
```

2. **Critical Section Placement**
```
.ram_functions : {
    *(.time_critical*)  /* Functions that need fast execution */
} > CCRAM
```

3. **Memory Usage Optimization**
```
.compressed : {
    *(.compressed*)
    KEEP(*(.compression_table))
} > FLASH
```

This chapter provides a comprehensive understanding of linker scripts and their practical applications. In the next chapter, we'll explore dynamic linking and how it adds flexibility to program loading and execution.
