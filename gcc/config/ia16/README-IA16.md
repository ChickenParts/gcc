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

---

## Binutils Requirements

⚠️ **IMPORTANT**: The GCC ia16 backend alone is **not sufficient** to build ia16 programs. You also need:

1. **GNU Binutils with ia16 support** - Assembler, linker, and utilities
2. **C library** (optional but recommended) - newlib or elks-libc

### Why You Need Binutils

GCC is only a compiler - it generates assembly code. To create executables, you need:

| Tool | Purpose | ia16-Specific Features |
|------|---------|----------------------|
| **as** | Assembler | Converts .s files to .o object files |
| **ld** | Linker | Links .o files into executables, handles relocations |
| **objcopy** | Object converter | Converts ELF to binary, creates ROM images |
| **objdump** | Disassembler | Debug and inspect ia16 binaries |
| **ar** | Archiver | Create static libraries (.a files) |

Without ia16-aware binutils, you'll get errors like:
- "ia16-elf-as: command not found"
- "ia16-elf-ld: unrecognized target format"
- Linker errors with ia16-specific relocations

## Building a Complete ia16 Toolchain

### Option 1: Build Everything (Recommended)

Use the automated build script from TK Chia:

```bash
# 1. Clone the build repository
git clone https://github.com/tkchia/build-ia16.git
cd build-ia16

# 2. Install prerequisites
sudo apt-get install build-essential texinfo bison flex \
  libgmp-dev libmpfr-dev libmpc-dev zlib1g-dev

# 3. Run the build script (this takes 1-2 hours)
./build.sh gcc1 binutils newlib gcc2

# This builds:
# - binutils-ia16 (assembler, linker, utilities)
# - gcc-ia16 (stage 1 bootstrap)
# - newlib (C library)
# - gcc-ia16 (stage 2 with full library support)

# 4. Install to /usr/local or custom prefix
./build.sh prefix=/opt/ia16 install
```

### Option 2: Build Binutils Separately

If you want to build binutils manually:

```bash
# 1. Clone binutils-ia16
git clone https://github.com/tkchia/binutils-ia16.git
cd binutils-ia16

# 2. Configure for ia16-elf target
mkdir build-ia16
cd build-ia16
../configure \
  --target=ia16-elf \
  --prefix=/opt/ia16 \
  --disable-werror \
  --disable-nls

# 3. Build
make -j$(nproc)

# 4. Install
sudo make install

# 5. Add to PATH
export PATH=/opt/ia16/bin:$PATH
```

### Option 3: Use Pre-built Packages (Ubuntu/Debian)

```bash
# Add PPA repository
sudo add-apt-repository ppa:tkchia/build-ia16
sudo apt-get update

# Install the complete toolchain
sudo apt-get install gcc-ia16-elf
```

### Verifying Your Installation

After installation, verify all tools are present:

```bash
# Check compiler
ia16-elf-gcc --version

# Check assembler
ia16-elf-as --version

# Check linker
ia16-elf-ld --version

# Check other utilities
ia16-elf-objcopy --version
ia16-elf-objdump --version
ia16-elf-ar --version
ia16-elf-nm --version
ia16-elf-strip --version
```

All should respond with version information and show "ia16-elf" as the target.

## What Binutils-ia16 Provides

### Key Features

1. **16-bit x86 Assembly Support**
   - Full 8086/80186/80286 instruction set
   - Segment register handling
   - Far call/jump support

2. **MS-DOS MZ Executable Format**
   - Proper DOS EXE header generation
   - Relocation table creation
   - PSP (Program Segment Prefix) support

3. **ELKS Object Format**
   - a.out format for ELKS
   - Kernel module support

4. **ELF Extensions**
   - 16-bit ELF relocations
   - Segment-based addressing
   - Far pointer relocations

5. **Linker Scripts**
   - DOS COM file layout (org 0x100)
   - DOS EXE file layout
   - ROM image layouts
   - Custom memory maps

## Linker Details

### Default Linker Scripts

The ia16 linker includes several built-in scripts:

```bash
# View available scripts
ia16-elf-ld --verbose

# Common scripts:
# - Default ELF layout (ia16-elf)
# - DOS COM layout (tiny model)
# - DOS EXE layout (small/medium model)
# - ELKS a.out layout
```

### Creating DOS COM Files with Linker

```bash
# Method 1: Using output format
ia16-elf-gcc -mcmodel=tiny hello.c -o hello.elf
ia16-elf-objcopy -O binary hello.elf hello.com

# Method 2: Direct binary output
ia16-elf-gcc -mcmodel=tiny \
  -Wl,--oformat=binary \
  -Wl,-Ttext=0x100 \
  -o hello.com hello.c
```

### Creating DOS EXE Files

The linker automatically creates proper MZ headers:

```bash
# Small model EXE
ia16-elf-gcc -mcmodel=small -o hello.exe hello.c

# The linker adds:
# - MZ signature (0x5A4D)
# - Relocation table
# - Header size calculation
# - Entry point setup
```

### Custom Linker Scripts

For special memory layouts (ROMs, embedded systems):

```ld
/* custom.ld - Custom linker script */
OUTPUT_FORMAT("binary")
ENTRY(_start)

MEMORY {
    ROM (rx)  : ORIGIN = 0xF000, LENGTH = 4K
    RAM (rwx) : ORIGIN = 0x0000, LENGTH = 64K
}

SECTIONS {
    .text : {
        *(.text)
    } > ROM

    .data : {
        *(.data)
    } > RAM

    .bss : {
        *(.bss)
    } > RAM
}
```

```bash
# Use custom script
ia16-elf-gcc -T custom.ld -o firmware.bin firmware.c
```

## Assembler Details

### Inline Assembly Syntax

The ia16 assembler supports standard AT&T and Intel syntax:

```c
// AT&T syntax (default in GCC)
__asm__ volatile(
    "movw $0x1234, %ax\n"
    "movw %ax, %ds\n"
    ::: "ax"
);

// Intel syntax
__asm__ volatile(
    ".intel_syntax noprefix\n"
    "mov ax, 0x1234\n"
    "mov ds, ax\n"
    ".att_syntax prefix\n"
    ::: "ax"
);
```

### Pure Assembly Files

```asm
# hello.s - Pure assembly hello world
.code16
.section .text
.global _start

_start:
    mov $0x09, %ah          # DOS print string
    lea message, %dx
    int $0x21               # Call DOS

    mov $0x4C, %ah          # DOS exit
    int $0x21

message:
    .ascii "Hello from assembly!$"
```

```bash
# Assemble and link
ia16-elf-as -o hello.o hello.s
ia16-elf-ld -o hello.com hello.o \
  --oformat=binary \
  -Ttext=0x100
```

## Object Format Conversions

### ELF to Binary (for COM/ROM)

```bash
# Compile to ELF first
ia16-elf-gcc -mcmodel=tiny -o program.elf program.c

# Convert to raw binary
ia16-elf-objcopy -O binary program.elf program.com

# Strip specific sections
ia16-elf-objcopy -O binary \
  -j .text -j .data -j .rodata \
  program.elf program.bin
```

### Creating ROM Images

```bash
# Create ROM with specific size
ia16-elf-objcopy -O binary program.elf program.rom

# Pad to exact size (e.g., 8KB)
truncate -s 8192 program.rom

# Verify size
ls -lh program.rom
```

### Extracting Sections

```bash
# Disassemble
ia16-elf-objdump -d program.o

# Show all sections
ia16-elf-objdump -h program.o

# Show symbols
ia16-elf-nm program.o

# Show relocations
ia16-elf-objdump -r program.o
```

## Relocation Types

The ia16 binutils support special relocations:

| Relocation | Description | Usage |
|------------|-------------|-------|
| R_386_16 | 16-bit absolute | Near pointers |
| R_386_PC16 | 16-bit PC-relative | Near calls/jumps |
| R_386_SEG16 | 16-bit segment | Far pointers (segment part) |
| R_386_PC32 | 32-bit PC-relative | Far calls (segment:offset) |

These are essential for:
- Far function calls
- Segment register loads
- Inter-segment jumps
- Position-independent code

## Building Without C Library

For bare-metal or BIOS code:

```bash
# Minimal startup code
cat > crt0.s << 'ASM'
.code16
.section .text
.global _start

_start:
    # Initialize stack
    mov $0x9000, %ax
    mov %ax, %ss
    mov $0xFFFE, %sp

    # Initialize data segment
    mov $0x1000, %ax
    mov %ax, %ds

    # Call main
    call main

    # Exit (varies by platform)
    jmp .
ASM

# Compile without standard libraries
ia16-elf-as -o crt0.o crt0.s
ia16-elf-gcc -ffreestanding -nostdlib \
  -o firmware.elf crt0.o main.c
ia16-elf-objcopy -O binary firmware.elf firmware.bin
```

## Library Creation

### Static Libraries

```bash
# Compile library sources
ia16-elf-gcc -c lib1.c -o lib1.o
ia16-elf-gcc -c lib2.c -o lib2.o

# Create archive
ia16-elf-ar rcs libmylib.a lib1.o lib2.o

# Use in linking
ia16-elf-gcc -o program.exe main.c -L. -lmylib
```

### Shared Libraries (Not Supported)

⚠️ Shared libraries (.so) are **not supported** in ia16 targets:
- No dynamic linker in DOS/ELKS
- 16-bit segmented memory model limitations
- Use static linking only

## Troubleshooting Linker Issues

### "undefined reference to _start"

```bash
# Solution: Provide entry point or use -nostartfiles
ia16-elf-gcc -nostartfiles -e main -o program.com program.c
```

### "section .text VMA overlaps .data"

```bash
# Solution: Adjust memory layout with linker script
# or change code model
ia16-elf-gcc -mcmodel=small -o program.exe program.c
```

### "relocation truncated to fit"

```bash
# Solution: Value too large for 16-bit offset
# Use far pointers or smaller memory model
ia16-elf-gcc -mcmodel=medium -o program.exe program.c
```

### "can't read relocation record"

```bash
# Solution: Incompatible object files (check bitness)
file *.o  # Should show "ia16" or "80386"
# Rebuild all objects with same toolchain
```

## Complete Build Example

Putting it all together - building a multi-file DOS program:

```bash
# Project structure:
# main.c
# utils.c
# utils.h
# Makefile

# Makefile
CC = ia16-elf-gcc
CFLAGS = -mcmodel=small -Os -Wall
LDFLAGS = 

OBJS = main.o utils.o

program.exe: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o program.exe

.PHONY: clean
```

## Resources

- **Binutils Source**: https://github.com/tkchia/binutils-ia16
- **Build Scripts**: https://github.com/tkchia/build-ia16
- **PPA Packages**: https://launchpad.net/~tkchia/+archive/ubuntu/build-ia16
- **Binutils Manual**: https://sourceware.org/binutils/docs/

## Summary

✅ **You must build/install binutils-ia16 before using GCC**
✅ **Use the build-ia16 automated script for easiest setup**
✅ **All standard binutils tools are supported**
✅ **Special relocations handle far pointers and segments**
✅ **Multiple output formats supported (ELF, binary, MZ)**

The ia16 GCC backend and binutils work together to provide a complete toolchain for 16-bit x86 development.

