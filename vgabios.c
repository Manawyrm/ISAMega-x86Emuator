#include "vgabios.h"

#define VBIOS_ROM 0xc0000
#define VBIOS_ROM_SIZE 0x10000
#define VBIOS_MEM 0xa0000
#define VBIOS_MEM_SIZE 0x10000
#define VBE_BUF 0x8000
uint8_t loaded_rom = 0;

void vm_write_byte(x86emu_t *emu, unsigned addr, unsigned val, unsigned perm)
{
	x86emu_write_byte_noperm(emu, addr, val);
	x86emu_set_perm(emu, addr, addr, perm | X86EMU_PERM_VALID);
}

struct
{
	struct
	{
		unsigned segment;
		unsigned offset;
	}
	start;
	unsigned load;
	unsigned max_instructions;
	unsigned bits_32: 1;
	char *file;
}
opt;

/*
 *Parse options, then run emulation.
 */
int main(int argc, char **argv)
{
	opt.start.segment = 0;
	opt.start.offset = opt.load = 0x7c00;
	opt.max_instructions = 0;

	emu_run("");

	return 0;
}

/*
 *Write emulation log to console.
 */
void flush_log(x86emu_t *emu, char *buf, unsigned size)
{
	if (!buf || !size) return;

	fwrite(buf, size, 1, stdout);
}

x86emu_t * globalemu;
void sigfunc(int sig)
{
	if (sig != SIGINT)
		return;
	else
	{
		//unsigned flags = X86EMU_RUN_NO_CODE | X86EMU_RUN_TIMEOUT;

		//x86emu_stop(globalemu);

		/*x86emu_dump(globalemu, X86EMU_DUMP_DEFAULT | X86EMU_DUMP_ACC_MEM);

			x86emu_clear_log(globalemu, 1);

			x86emu_run(globalemu, flags);
*/
	}
}

/*
 *Create new emulation object.
 */
x86emu_t* emu_new()
{
	x86emu_t *emu = x86emu_new(X86EMU_PERM_RWX, X86EMU_PERM_RWX);

	/*log buf size of 1000000 is purely arbitrary */
	x86emu_set_log(emu, 10000000, flush_log);

	emu->log.trace = X86EMU_TRACE_DEFAULT;

	return emu;
}

x86emu_memio_handler_t old_memio;

unsigned emu_memio_handler(x86emu_t *emu, u32 addr, u32 *val, unsigned type)
{
	unsigned bits = type &0xff;
	unsigned mytype = type &~0xff;

	if (mytype == X86EMU_MEMIO_O)
	{
		printf("[memio] output! Addr: %08lx Val: %08lx\n", (unsigned long) addr, (unsigned long) *val);
		ioWrite(addr, *val);
		//ioWrite(0x80, *val);
		return 0;
	}

	if (mytype == X86EMU_MEMIO_I)
	{
		uint8_t data = ioRead(addr);
		printf("[memio] input! Addr: %08lx Read value: %02x\n", (unsigned long) addr, data);

		*val = data;

		return 0;
	}

	if (mytype == X86EMU_MEMIO_R || mytype == X86EMU_MEMIO_W || mytype == X86EMU_MEMIO_X)
	{
		if (/*(addr >= 0x000A0000 && addr <= 0x000BFFFF) || */ (addr >= 0x000C0000 && addr <= 0x000C7FFF && loaded_rom == 0))
		{
			//printf("[memiorw] debug addr: %08lx Data:%02x\n", addr,*val);
			//x86emu_set_perm(emu, addr, addr, X86EMU_PERM_RWX | X86EMU_PERM_VALID);

			switch (bits)
			{
				case X86EMU_MEMIO_8_NOPERM:
				case X86EMU_MEMIO_8:
					if (mytype == X86EMU_MEMIO_R || mytype == X86EMU_MEMIO_X)
					{
						uint8_t data = memRead(addr);
						*val = data;

						printf("[memiorw] 8 RX addr: %08lx Data:%02x\n", (unsigned long) addr, data);
					}
					if (mytype == X86EMU_MEMIO_W)
					{
						memWrite(addr, *val);
						printf("[memiorw] 8 W addr: %08lx Data:%02x\n", (unsigned long) addr, *val);
					}
					return 0;
					break;
				case X86EMU_MEMIO_16:
					if (mytype == X86EMU_MEMIO_R || mytype == X86EMU_MEMIO_X)
					{
						uint16_t data = memRead(addr);
						data |= memRead(addr + 1) << 8;
						*val = data;

						printf("[memiorw] 16 RX addr: %08lx Data:%04x\n", (unsigned long) addr, data);
					}
					if (mytype == X86EMU_MEMIO_W)
					{
						memWrite(addr, *val & 0xFF);
						memWrite(addr + 1, (*val >> 8) & 0xFF);
						printf("[memiorw] 16 W addr: %08lx Data:%04x\n", (unsigned long) addr, *val);
					}
					return 0;
					break;
				case X86EMU_MEMIO_32:
					printf("[memiorw] 32 addr: %08lx\n", (unsigned long) addr);
					assert(0);
					break;
			}
		}
	}

	return old_memio(emu, addr, val, type);
}

int emu_int_handler(x86emu_t *emu, u8 num, unsigned type)
{
	// Int 10h, BIOS Video interrupt
	if (num == 0x10)
	{
		// if there's no interrupt in the IVT, 
		if (x86emu_read_byte(emu, 0x40) == 0x00)
		{
			// we'll just set AL to 12 as a "valid" return for each function.
			// this is expected by Trident video ROMs. 

			printf("[int] a: %08x b: %08x\n", emu->x86.gen.A.I32_reg.e_reg, emu->x86.gen.B.I32_reg.e_reg);
			//[int] a: 00001201 b: 00000032

			// AL=12 Function supported
			emu->x86.gen.A.I32_reg.e_reg &= 0xFFFFFF00UL;
			emu->x86.gen.A.I32_reg.e_reg |= 0x12UL;

			return 1;
		}
	}

	return 0;
}

/*
 * Setup registers and memory.
 */
int emu_init(x86emu_t *emu, char *file)
{
	//signal(SIGINT, sigfunc);
	globalemu = emu;

	init_serial();

	printf("serial ISA init done, giving the ISA card 5 seconds time after reset...");
	sleep(5);

	old_memio = x86emu_set_memio_handler(emu, emu_memio_handler);
	x86emu_set_intr_handler(emu, emu_int_handler);

	// Optional: Load a VGA BIOS from a file and write it to 0xC0000
	/*
	{
		FILE * f;
		if(!(f = fopen(file, "r"))) return 0;
		 loaded_rom = 1; 

		 addr = VBIOS_ROM;
		 while((i = fgetc(f)) != EOF) {
			 x86emu_write_byte(emu, addr, i);
			 x86emu_set_perm(emu, addr, addr++, X86EMU_PERM_RWX | X86EMU_PERM_VALID);
		 }
		 fclose(f);
	 }
	 */

	printf("video bios: size 0x%04x\n", x86emu_read_byte(emu, VBIOS_ROM + 2) * 0x200);
	printf("video bios: entry 0x%04x:0x%04x\n",
		x86emu_read_word(emu, 0x10 * 4 + 2),
		x86emu_read_word(emu, 0x10 * 4)
	);

	return 1;
}

/*
 * Run emulation.
 */
void emu_run(char *file)
{
	x86emu_t *emu = emu_new();
	unsigned flags = X86EMU_RUN_NO_CODE | X86EMU_RUN_TIMEOUT;
	int ok = 0;

	if (!file) return;

	ok = emu_init(emu, file);

	if (ok)
	{
		printf("*** running %s  ***\n\n", file);

		// stack &buffer space
		x86emu_set_perm(emu, VBE_BUF, 0xffff, X86EMU_PERM_RW);

		// start address 0:0x7c00 (where the MBR would normally be executed)
		// we'll do a far call to 0x0xC0003 (aka 0C00:0003) here. 
		x86emu_set_seg_register(emu, emu->x86.R_CS_SEL, 0);
		emu->x86.R_EIP = 0x7c00;

		uint8_t code[5] = "\x9a\x03\x00\x00\xc0";
		for (int i = 0; i < sizeof(code); ++i)
		{
			vm_write_byte(emu, 0x7c00 + i, code[i], X86EMU_PERM_RX);
		}
		// and place a HLT instruction after the far call so the emulator stops after 
		// we're done executing the video ROM.
		vm_write_byte(emu, 0x7c00 + sizeof(code), 0xf4, X86EMU_PERM_RX);

		emu->max_instr = opt.max_instructions;
		x86emu_run(emu, flags);

		x86emu_dump(emu, X86EMU_DUMP_DEFAULT | X86EMU_DUMP_ACC_MEM);

		x86emu_clear_log(emu, 1);

		// Optional: 
		printf("setting video mode to 0x03\n");

		// set video mode to 0x03 (80x25)
		x86emu_set_seg_register(emu, emu->x86.R_CS_SEL, 0);
		emu->x86.R_EIP = 0x7c00;

		uint8_t code2[6] = "\xb4\x00\xb0\x03\xcd\x10";
		for (int i = 0; i < sizeof(code2); ++i)
		{
			vm_write_byte(emu, 0x7c00 + i, code2[i], X86EMU_PERM_RX);
		}
		vm_write_byte(emu, 0x7c00 + sizeof(code2), 0xf4, X86EMU_PERM_RX);

		emu->max_instr = opt.max_instructions;
		x86emu_run(emu, flags);

		x86emu_dump(emu, X86EMU_DUMP_DEFAULT | X86EMU_DUMP_ACC_MEM);

		x86emu_clear_log(emu, 1);

		printf("finished cleanly\n");
	}

	x86emu_done(emu);
}