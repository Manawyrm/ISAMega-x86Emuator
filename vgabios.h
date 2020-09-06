#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <x86emu.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()
#include <assert.h>
#include <signal.h>
#include "serialisa.h"

void flush_log(x86emu_t *emu, char *buf, unsigned size);
x86emu_t* emu_new(void);
int emu_init(x86emu_t *emu, char *file);
void emu_run(char *file);
void vm_write_byte(x86emu_t *emu, unsigned addr, unsigned val, unsigned perm);
void vm_write_word(x86emu_t *emu, unsigned addr, unsigned val, unsigned perm);
