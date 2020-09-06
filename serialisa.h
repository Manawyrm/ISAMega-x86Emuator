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
#include <arpa/inet.h>

void init_serial();
void ioWrite(uint32_t address, uint8_t data);
uint8_t ioRead(uint32_t address);
void memWrite(uint32_t address, uint8_t data);
uint8_t memRead(uint32_t address);