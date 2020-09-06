#include "serialisa.h"

int serial;

void init_serial()
{
	serial = open("/dev/ttyUSB0", O_RDWR);

	if (serial < 0)
	{
		printf("Error %i from open: %s\n", errno, strerror(errno));
	}

	struct termios tty;

	// Read in existing settings, and handle any error
	if (tcgetattr(serial, &tty) != 0)
	{
		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS;  // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL;  // Turn on READ &ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE;  // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);  // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST;  // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR;  // Prevent conversion of newline to carriage return/line feed

	tty.c_cc[VTIME] = 0;  // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;

	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);

	if (tcsetattr(serial, TCSANOW, &tty) != 0)
	{
		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}

	// wait for readyness
	char read_buf[256];
	while (1)
	{
		int n = read(serial, &read_buf, sizeof(read_buf));
		for (int i = 0; i < n; ++i)
		{
			if (read_buf[i] == 'R')
			{
				//printf("%s\n", read_buf);
				printf("hardware init done.\n");
				return;
			}
		}
	}
}

void ioWrite(uint32_t address, uint8_t data)
{
	uint8_t buffer[100];
	uint16_t length = snprintf(buffer, 100, "o%04X%02X\n", address, data);
	write(serial, buffer, length);

	//printf("%s\n", buffer);
	//printf("[outb] Addr: %04lx, Data: %02lx\n", address, data);

	// wait for return 
	char read_buf[256];
	while (1)
	{
		int n = read(serial, &read_buf, sizeof(read_buf));
		for (int i = 0; i < n; ++i)
		{
			if (read_buf[i] == 'o')
			{
				return;
			}
		}
	}
}

uint8_t ioRead(uint32_t address)
{
	uint8_t buffer[100];
	uint16_t length = snprintf(buffer, 100, "i%04X\n", address);
	write(serial, buffer, length);

	//printf("%s\n", buffer);
	//printf("[inb] Addr: %04lx, Data: %02lx\n", address);

	// wait for return 
	char read_buf[256];
	uint16_t offset = 0;
	while (1)
	{
		int n = read(serial, read_buf + offset, (sizeof(read_buf) - offset));
		for (int i = 0; i < (offset + n); ++i)
		{
			if (read_buf[i] == '\n')
			{
				read_buf[i + 1] = 0x00;
				//printf("%s\n", read_buf);
				return strtol(read_buf + 1, NULL, 16);
			}
		}
		offset += n;
	}
}


void memWrite(uint32_t address, uint8_t data)
{
	uint8_t buffer[100];
	uint16_t length = snprintf(buffer, 100, "w%06X%02X\n", address, data);
	write(serial, buffer, length);

	//printf("%s\n", buffer);

	// wait for return 
	char read_buf[256];
	while (1)
	{
		int n = read(serial, &read_buf, sizeof(read_buf));
		for (int i = 0; i < n; ++i)
		{
			if (read_buf[i] == 'w')
			{
				return;
			}
		}
	}
}

uint8_t memRead(uint32_t address)
{
	uint8_t buffer[100];
	uint16_t length = snprintf(buffer, 100, "r%06X\n", address);
	write(serial, buffer, length);

	//printf("%s\n", buffer);

	// wait for return 
	char read_buf[256];
	uint16_t offset = 0;
	while (1)
	{
		int n = read(serial, read_buf + offset, (sizeof(read_buf) - offset));
		for (int i = 0; i < (offset + n); ++i)
		{
			if (read_buf[i] == '\n')
			{
				read_buf[i + 1] = 0x00;
				//printf("%s\n", read_buf);
				return strtol(read_buf + 1, NULL, 16);
			}
		}
		offset += n;
	}
}
