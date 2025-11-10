# GCC IA-16 Backend Documentation

## Overview

The IA-16 backend for GCC provides support for generating 16-bit x86 code targeting Intel 8086/8088, 80186/80188, 80286, and compatible processors (NEC V20/V30). This port enables development of:

- **DOS COM files** - Single-segment executables
- **DOS EXE files** - Multi-segment executables
- **DOS extender programs** - Protected mode applications (DPMI/DOSX)
- **BIOS Option ROMs** - x86 firmware
- **ELKS programs** - Executables for Embeddable Linux Kernel Subset
- **Freestanding code** - Bare-metal applications

This backend was originally developed by Rask Ingemann Lambertsen and Andrew Jenner, with significant enhancements by TK Chia including far pointer support. It has been ported from GCC 6.3.0 to the current GCC codebase.

## Source Information

- **Original repository**: https://github.com/tkchia/gcc-ia16
- **Based on**: GCC 6.3.0
- **Current port**: Adapted for GCC 15+
- **Key contributors**: Rask Ingemann Lambertsen, Andrew Jenner, TK Chia

## Build Status

✅ **API Compatibility**: All deprecated GCC 6.x APIs updated to GCC 15+ equivalents
✅ **Target Hooks**: Converted deprecated macros to modern target hooks
✅ **Build System**: Configured for ia16-elf and ia16-elks targets
✅ **Library Support**: libgcc runtime support included

## Building the IA-16 GCC Compiler

### Prerequisites

You need the standard GCC build requirements:
- GMP 4.2+
- MPFR 3.1.0+
- MPC 0.8.0+
- Build tools (make, binutils, etc.)

### Build Steps

```bash
# 1. Create build directory
mkdir build-ia16-gcc
cd build-ia16-gcc

# 2. Configure for ia16-elf target
../configure \
  --target=ia16-elf \
  --prefix=/opt/ia16 \
  --enable-languages=c,c++ \
  --disable-libssp \
  --disable-libquadmath \
  --with-newlib

# 3. Build the compiler
make -j$(nproc)

# 4. Install
sudo make install

# 5. Add to PATH
export PATH=/opt/ia16/bin:$PATH
```

### Alternative: ELKS Target

For ELKS (Embeddable Linux Kernel Subset) development:

```bash
../configure \
  --target=ia16-elks \
  --prefix=/opt/ia16-elks \
  --enable-languages=c \
  --disable-libssp
```

## Architecture Support

### Processor Options

| Option | Description |
|--------|-------------|
| `-march=i8086` | Intel 8086 (default) |
| `-march=i8088` | Intel 8088 |
| `-march=i80186` | Intel 80186 |
| `-march=i80188` | Intel 80188 |
| `-march=i80286` | Intel 80286 |
| `-march=any_186` | 80186 or V20/V30 |
| `-march=v20` | NEC V20 |
| `-march=v30` | NEC V30 |
| `-march=v30mz` | NEC V30MZ |

### Tuning Options

Similar to `-march`, but only affects optimization:
- `-mtune=i8086`, `-mtune=i80186`, `-mtune=i80286`, etc.

## Memory Models

### Code Models

| Model | Description | Use Case |
|-------|-------------|----------|
| `-mcmodel=tiny` | Single segment for code and data (default) | COM files, small programs |
| `-mcmodel=small` | Separate code and data segments | DOS EXE files |
| `-mcmodel=medium` | Multiple code segments, one data segment | Large programs |

### Example Usage

```bash
# Tiny model (COM file)
ia16-elf-gcc -mcmodel=tiny -o hello.com hello.c

# Small model (EXE file)
ia16-elf-gcc -mcmodel=small -o hello.exe hello.c

# Medium model (large program)
ia16-elf-gcc -mcmodel=medium -o large.exe large.c
```

## Creating DOS COM Files

COM files are single-segment executables limited to 64KB that load at offset 0x100.

### Basic COM File

```c
// hello.c
void _start(void) {
    __asm__(
        "mov $0x09, %%ah\n"
        "lea message, %%dx\n"
        "int $0x21\n"
        "mov $0x4C, %%ah\n"
        "int $0x21\n"
        "message: .ascii \"Hello, World!$\"\n"
        ::: "ax", "dx"
    );
}
```

### Build Command

```bash
ia16-elf-gcc -mcmodel=tiny -ffreestanding -nostdlib \
  -Wl,--oformat=binary \
  -Wl,-Ttext=0x100 \
  -o hello.com hello.c
```

### With C Library

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("Hello from COM file!\n");
    return 0;
}
```

```bash
ia16-elf-gcc -mcmodel=tiny -o hello.com hello.c
```

## Creating DOS EXE Files

EXE files support multiple segments and relocations, allowing programs larger than 64KB.

### Basic EXE File

```c
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    printf("Hello from EXE file!\n");
    printf("Arguments: %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("  argv[%d] = %s\n", i, argv[i]);
    }
    return 0;
}
```

### Build Command

```bash
ia16-elf-gcc -mcmodel=small -o hello.exe hello.c
```

### Large Programs (Medium Model)

```bash
ia16-elf-gcc -mcmodel=medium -o bigprog.exe bigprog.c module1.c module2.c
```

## Creating BIOS Option ROMs

BIOS option ROMs are firmware extensions that execute during system boot. They must:
- Start with signature `0x55 0xAA`
- Have size in 512-byte blocks at offset 2
- Have initialization code at offset 3
- Include a checksum that makes the entire ROM sum to 0

### Option ROM Template

```c
// optrom.c - BIOS Option ROM template
#include <stdint.h>

// ROM header structure
struct rom_header {
    uint16_t signature;     // Must be 0xAA55
    uint8_t  size_blocks;   // Size in 512-byte blocks
    uint8_t  entry_point[3]; // Entry point (usually near jump)
} __attribute__((packed));

// ROM initialization function
void __attribute__((section(".init"))) rom_init(void) {
    // Your initialization code here
    __asm__ volatile(
        "push %%ds\n"
        "push %%bx\n"
        // ... do ROM initialization ...
        "pop %%bx\n"
        "pop %%ds\n"
        "retf\n"  // Far return to BIOS
        ::: "memory"
    );
}

// Main ROM entry
void _start(void) {
    rom_init();
}
```

### Linker Script for Option ROM

```ld
/* optrom.ld */
OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS {
    . = 0xC000; /* Common option ROM base address */

    .text : {
        /* ROM signature */
        SHORT(0xAA55);

        /* Size in 512-byte blocks (will be patched) */
        BYTE(0x00);

        /* Entry point - jump to init */
        *(.init)
        *(.text)
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    /* Ensure ROM size is multiple of 512 */
    . = ALIGN(512);
}
```

### Build Commands

```bash
# Compile
ia16-elf-gcc -mcmodel=small -ffreestanding -nostdlib -c optrom.c -o optrom.o

# Link with custom linker script
ia16-elf-ld -T optrom.ld optrom.o -o optrom.tmp

# Convert to binary
ia16-elf-objcopy -O binary optrom.tmp optrom.rom

# Calculate and patch size
SIZE=$(stat -c%s optrom.rom)
BLOCKS=$(( (SIZE + 511) / 512 ))
printf "\\x$(printf %x $BLOCKS)" | dd of=optrom.rom bs=1 seek=2 count=1 conv=notrunc

# Calculate and patch checksum (Python script)
python3 << 'EOF'
with open('optrom.rom', 'r+b') as f:
    data = bytearray(f.read())
    checksum = (256 - sum(data) % 256) % 256
    # Append checksum at end
    data.append(checksum)
    f.seek(0)
    f.write(data)
EOF

echo "Option ROM created: optrom.rom"
```

### Testing Option ROM

```bash
# Test in QEMU
qemu-system-i386 -option-rom optrom.rom -nographic

# Or in Bochs
bochs -q 'optromimage: file=optrom.rom'
```

### Common Option ROM Addresses

| Address | Usage |
|---------|-------|
| 0xC000:0 | VGA BIOS |
| 0xC800:0 | Hard disk controller |
| 0xD000:0 | Network boot ROM |
| 0xE000:0 | System expansion |

## Calling Conventions

### Standard (cdecl)

Default calling convention:
- Arguments pushed right-to-left on stack
- Caller cleans up stack
- Return value in AX (or DX:AX for 32-bit)

```c
int add(int a, int b) {  // Standard cdecl
    return a + b;
}
```

### Stdcall (`-mrtd`)

Callee cleans up stack:

```bash
ia16-elf-gcc -mrtd -o prog.exe prog.c
```

```c
int __attribute__((stdcall)) add(int a, int b) {
    return a + b;
}
```

### Register Parameter Calling (`-mregparmcall`)

Passes some arguments in registers:

```bash
ia16-elf-gcc -mregparmcall -o prog.exe prog.c
```

Arguments passed in AX, DX, CX (then stack for remainder).

### Far Pointers

For programs with multiple segments:

```c
// Far pointer type
char __far *ptr;

// Far function
void __far my_far_function(void) {
    // Function in different code segment
}

// Calling far function
void (*far_fn)(void) __far = my_far_function;
far_fn();
```

## Runtime Environments

### MS-DOS (Default)

```bash
ia16-elf-gcc -o hello.exe hello.c
# or explicitly:
ia16-elf-gcc -mr=msdos -o hello.exe hello.c
```

### ELKS (Embeddable Linux Kernel Subset)

```bash
ia16-elf-gcc -mr=elks -o hello hello.c
# or use the melks shorthand:
ia16-elf-gcc -melks -o hello hello.c
```

### DOS Extender (DPMI)

For protected mode programs:

```bash
ia16-elf-gcc -mdosx -o hello.exe hello.c
```

### Freestanding (No OS)

For bare-metal or firmware:

```bash
ia16-elf-gcc -ffreestanding -nostdlib -o firmware.bin firmware.c
```

## Complete Examples

### Example 1: Simple DOS COM Program

```c
// simple.c
#include <stdio.h>

int main(void) {
    puts("Hello from 16-bit world!");
    return 0;
}
```

```bash
ia16-elf-gcc -mcmodel=tiny -o simple.com simple.c
```

### Example 2: DOS EXE with File I/O

```c
// fileio.c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *fp = fopen("test.txt", "w");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    fprintf(fp, "Written from IA-16 GCC!\n");
    fclose(fp);

    printf("File written successfully.\n");
    return 0;
}
```

```bash
ia16-elf-gcc -mcmodel=small -o fileio.exe fileio.c
```

### Example 3: BIOS Interrupt

```c
// biosint.c - Direct BIOS calls
#include <stdio.h>

void print_char_bios(char c) {
    __asm__ volatile(
        "mov $0x0E, %%ah\n"  // BIOS teletype output
        "mov %0, %%al\n"
        "int $0x10\n"
        : : "r"(c) : "ax"
    );
}

int main(void) {
    const char *msg = "Hello via BIOS!\n";
    while (*msg) {
        print_char_bios(*msg++);
    }
    return 0;
}
```

```bash
ia16-elf-gcc -mcmodel=tiny -o biosint.com biosint.c
```

### Example 4: TSR (Terminate and Stay Resident)

```c
// tsr.c - Simple TSR example
#include <dos.h>
#include <stdio.h>

void __attribute__((interrupt)) keyboard_handler(void) {
    // TSR keyboard handler
    __asm__ volatile("iret");
}

int main(void) {
    printf("Installing TSR...\n");

    // Install keyboard interrupt handler
    // ... (setup code) ...

    // Terminate and stay resident
    __asm__ volatile(
        "mov $0x3100, %%ax\n"  // TSR exit
        "int $0x21\n"
        ::: "ax"
    );

    return 0;  // Never reached
}
```

```bash
ia16-elf-gcc -mtsr -mcmodel=small -o tsr.exe tsr.c
```

### Example 5: Protected Mode (DOS Extender)

```c
// pmode.c - Protected mode example
#include <stdio.h>

int main(void) {
    printf("Running in protected mode!\n");
    printf("Can access extended memory.\n");

    // DPMI calls available here
    return 0;
}
```

```bash
ia16-elf-gcc -mdosx -o pmode.exe pmode.c
```

## Advanced Features

### Inline Assembly

```c
int read_port(int port) {
    int value;
    __asm__ volatile(
        "in %1, %0"
        : "=a"(value)
        : "d"(port)
    );
    return value;
}

void write_port(int port, int value) {
    __asm__ volatile(
        "out %0, %1"
        : : "a"(value), "d"(port)
    );
}
```

### Segment Overrides

```c
// Access video memory
void write_video(int offset, char ch) {
    __asm__ volatile(
        "mov $0xB800, %%ax\n"
        "mov %%ax, %%es\n"
        "mov %0, %%es:(%1)\n"
        : : "r"(ch), "r"(offset) : "ax", "es"
    );
}
```

### Far Pointers and Segments

```c
#include <stdio.h>

// Define far pointer type
typedef unsigned char __far *far_ptr;

// Read from far memory
unsigned char read_far(unsigned short seg, unsigned short off) {
    far_ptr ptr = (far_ptr)((unsigned long)seg << 16 | off);
    return *ptr;
}

// Segment selectors (protected mode)
void use_segments(void) {
    unsigned short code_seg, data_seg;

    __asm__ volatile(
        "mov %%cs, %0\n"
        "mov %%ds, %1\n"
        : "=r"(code_seg), "=r"(data_seg)
    );

    printf("CS=0x%04X DS=0x%04X\n", code_seg, data_seg);
}
```

## Optimization Options

### Size Optimization

```bash
# Optimize for size
ia16-elf-gcc -Os -mcmodel=tiny -o small.com prog.c

# Aggressive size optimization
ia16-elf-gcc -Os -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -o tiny.com prog.c
```

### Speed Optimization

```bash
# Optimize for speed
ia16-elf-gcc -O2 -mcmodel=small -o fast.exe prog.c

# Maximum optimization
ia16-elf-gcc -O3 -march=i80286 -o fastest.exe prog.c
```

## Debugging

### GDB Support

```bash
# Compile with debug symbols
ia16-elf-gcc -g -mcmodel=small -o prog.exe prog.c

# Debug with GDB
ia16-elf-gdb prog.exe
```

### QEMU Debugging

```bash
# Run in QEMU with GDB server
qemu-system-i386 -s -S -fda program.img

# In another terminal, connect GDB
ia16-elf-gdb program.exe
(gdb) target remote localhost:1234
(gdb) break main
(gdb) continue
```

## Limitations and Caveats

1. **Segment Limits**: 16-bit pointers limited to 64KB per segment
2. **Integer Sizes**: `int` is 16-bit, `long` is 32-bit
3. **Pointer Arithmetic**: Near pointers can't span segments
4. **Stack Size**: Limited stack space (typically 4KB-64KB)
5. **No Floating Point**: By default (use `-mfpmath=387` for FPU)
6. **Memory Models**: Must match at link time

## Troubleshooting

### Common Issues

**Link Error: "relocation truncated to fit"**
- Solution: Use `-mcmodel=medium` or split code into smaller modules

**Stack Overflow**
- Solution: Reduce local variables, use dynamic allocation, or increase stack size

**Far Pointer Issues**
- Solution: Use `__far` qualifier consistently, check segment registers

**Undefined Reference to `_start`**
- Solution: Add `-nostdlib` when not using standard library, provide `_start` function

## References

- [Original gcc-ia16 repository](https://github.com/tkchia/gcc-ia16)
- [GCC Internals Manual](https://gcc.gnu.org/onlinedocs/gccint/)
- [Intel 8086 Family User's Manual](https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf)
- [DOS Interrupt List](http://www.ctyme.com/intr/int.htm)

## License

This backend is part of GCC and licensed under GPL-3.0-or-later.

## Contributing

Contributions should follow GCC coding standards. Major changes should be discussed on the GCC mailing list.

---

**Last Updated**: November 2025
**Backend Version**: Ported from GCC 6.3.0 to GCC 15+
**Status**: Ready for use, actively maintained
