#ifndef _USER_HELPER
#define _USER_HELPER

#include "stdtype.h"
#include "fat32.h"
#include "stdmem.h"

#define NULL 0

#define BIOS_COLOR_BLUE     0x09
#define BIOS_COLOR_GREEN    0x0A
#define BIOS_COLOR_CYAN     0x0B
#define BIOS_COLOR_RED      0x0C
#define BIOS_COLOR_WHITE    0x0F

#define KEYBOARD_BUFFER_SIZE    256

struct ShellState
{
    uint32_t                    current_cluster_number;
    uint32_t                    retcode;
    uint32_t                    path_length;

    char                        path[KEYBOARD_BUFFER_SIZE];
}__attribute__((packed));

extern struct ShellState shell_state;

char *strtok(char *str, const char *delim);

uint32_t strlen(const char *str);

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

uint32_t parse_pathing(char *path, uint8_t is_changing, uint8_t ignore_last);

void puts(char *buf, uint8_t color);

#endif