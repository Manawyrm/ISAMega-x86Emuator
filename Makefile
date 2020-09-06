CC ?= gcc
RM = rm -f

CCFLAGS = -Wall -fomit-frame-pointer -O2 -D_GNU_SOURCE -ggdb

#DEPS = libx86emu
#DEPFLAGS_CC = `pkg-config --cflags $(DEPS)`
#DEPFLAGS_LD = `pkg-config --libs $(DEPS)`
DEPFLAGS_LD = -lx86emu
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
HDRS = $(wildcard *.h)

all: vgaemu

%.o : %.c $(HDRS) Makefile
	$(CC) -c $(CCFLAGS) $(DEPFLAGS_CC) $< -o $@

vgaemu: $(OBJS)
	$(CC) $(CCFLAGS) $^ $(DEPFLAGS_LD) -o vgaemu

clean:
	$(RM) $(OBJS)
	$(RM) vgaemu

.PHONY: all clean
