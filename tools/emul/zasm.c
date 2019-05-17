#include <stdint.h>
#include <stdio.h>
#include "libz80/z80.h"
#include "zasm-kernel.h"
#include "zasm-user.h"
#include "zasm-includes.h"

/* zasm reads from a specified blkdev, assemble the file and writes the result
 * in another specified blkdev. In our emulator layer, we use stdin and stdout
 * as those specified blkdevs.
 *
 * Because the input blkdev needs support for Seek, we buffer it in the emulator
 * layer.
 *
 * Memory layout:
 *
 * 0x0000 - 0x3fff: ROM code from zasm_glue.asm
 * 0x4000 - 0x47ff: RAM for kernel and stack
 * 0x4800 - 0x57ff: Userspace code
 * 0x5800 - 0xffff: Userspace RAM
 *
 * I/O Ports:
 *
 * 0 - stdin / stdout
 * 1 - When written to, rewind stdin buffer to the beginning.
 */

// in sync with zasm_glue.asm
#define USER_CODE 0x4800
#define STDIO_PORT 0x00
#define STDIN_SEEK_PORT 0x01
#define FS_DATA_PORT 0x02
#define FS_SEEK_PORT 0x03

// Other consts
#define STDIN_BUFSIZE 0x8000
// When defined, we dump memory instead of dumping expected stdout
//#define MEMDUMP
//#define DEBUG

static Z80Context cpu;
static uint8_t mem[0x10000];
// STDIN buffer, allows us to seek and tell
static uint8_t inpt[STDIN_BUFSIZE];
static int inpt_size;
static int inpt_ptr;
static uint8_t middle_of_seek_tell = 0;

static uint8_t fsdev[0xffff] = {0};
static uint16_t fsdev_size = 0;
static uint16_t fsdev_ptr = 0;
static uint8_t fsdev_middle_of_seek_tell = 0;

static uint8_t io_read(int unused, uint16_t addr)
{
    addr &= 0xff;
    if (addr == STDIO_PORT) {
        if (inpt_ptr < inpt_size) {
            return inpt[inpt_ptr++];
        } else {
            return 0;
        }
    } else if (addr == STDIN_SEEK_PORT) {
        if (middle_of_seek_tell) {
            middle_of_seek_tell = 0;
            return inpt_ptr & 0xff;
        } else {
#ifdef DEBUG
            fprintf(stderr, "tell %d\n", inpt_ptr);
#endif
            middle_of_seek_tell = 1;
            return inpt_ptr >> 8;
        }
    } else if (addr == FS_DATA_PORT) {
        if (fsdev_ptr < fsdev_size) {
            return fsdev[fsdev_ptr++];
        } else {
            return 0;
        }
    } else if (addr == FS_SEEK_PORT) {
        if (fsdev_middle_of_seek_tell) {
            fsdev_middle_of_seek_tell = 0;
            return fsdev_ptr & 0xff;
        } else {
#ifdef DEBUG
            fprintf(stderr, "FS tell %d\n", fsdev_ptr);
#endif
            fsdev_middle_of_seek_tell = 1;
            return fsdev_ptr >> 8;
        }
    } else {
        fprintf(stderr, "Out of bounds I/O read: %d\n", addr);
        return 0;
    }
}

static void io_write(int unused, uint16_t addr, uint8_t val)
{
    addr &= 0xff;
    if (addr == STDIO_PORT) {
// When mem-dumping, we don't output regular stuff.
#ifndef MEMDUMP
        putchar(val);
#endif
    } else if (addr == STDIN_SEEK_PORT) {
        if (middle_of_seek_tell) {
            inpt_ptr |= val;
            middle_of_seek_tell = 0;
#ifdef DEBUG
            fprintf(stderr, "seek %d\n", inpt_ptr);
#endif
        } else {
            inpt_ptr = (val << 8) & 0xff00;
            middle_of_seek_tell = 1;
        }
    } else if (addr == FS_DATA_PORT) {
        if (fsdev_ptr < fsdev_size) {
            fsdev[fsdev_ptr++] = val;
        }
    } else if (addr == FS_SEEK_PORT) {
        if (fsdev_middle_of_seek_tell) {
            fsdev_ptr |= val;
            fsdev_middle_of_seek_tell = 0;
#ifdef DEBUG
            fprintf(stderr, "FS seek %d\n", fsdev_ptr);
#endif
        } else {
            fsdev_ptr = (val << 8) & 0xff00;
            fsdev_middle_of_seek_tell = 1;
        }
    } else {
        fprintf(stderr, "Out of bounds I/O write: %d / %d\n", addr, val);
    }
}

static uint8_t mem_read(int unused, uint16_t addr)
{
    return mem[addr];
}

static void mem_write(int unused, uint16_t addr, uint8_t val)
{
    mem[addr] = val;
}

int main()
{
    // initialize memory
    for (int i=0; i<sizeof(KERNEL); i++) {
        mem[i] = KERNEL[i];
    }
    for (int i=0; i<sizeof(USERSPACE); i++) {
        mem[i+USER_CODE] = USERSPACE[i];
    }
    for (int i=0; i<sizeof(FSDEV); i++) {
        fsdev[i] = FSDEV[i];
    }
    fsdev_size = sizeof(FSDEV);
    // read stdin in buffer
    inpt_size = 0;
    inpt_ptr = 0;
    int c = getchar();
    while (c != EOF) {
        inpt[inpt_ptr] = c & 0xff;
        inpt_ptr++;
        if (inpt_ptr == STDIN_BUFSIZE) {
            break;
        }
        c = getchar();
    }
    inpt_size = inpt_ptr;
    inpt_ptr = 0;
    Z80RESET(&cpu);
    cpu.ioRead = io_read;
    cpu.ioWrite = io_write;
    cpu.memRead = mem_read;
    cpu.memWrite = mem_write;

    while (!cpu.halted) {
        Z80Execute(&cpu);
    }
#ifdef MEMDUMP
    for (int i=0; i<0x10000; i++) {
        putchar(mem[i]);
    }
#endif
    fflush(stdout);
#ifdef DEBUG
    fprintf(stderr, "Ended with A=%d DE=%d\n", cpu.R1.br.A, cpu.R1.wr.DE);
#endif
    return 0;
}

