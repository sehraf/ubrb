#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

#define debug 0

#define maxBufferSize ((0x1 << 10*2) * 10) // 10MB max size
#define chunk 2 // super slow for Digispark SerialUSB
#define sleepTime 100 // ms

#if debug
    #define debug_printf(args...) printf(args)
#else
    #define debug_printf(args...) do {} while(0)
#endif


int serial_open(char *serial_name, speed_t baud);
void serial_close(int fd);

int serial_read(int serial_fd, uint8_t *data, int size, int timeout_usec);
void serial_write(int serial_fd, char *data, int size);

void usage(char *argv[]);

int main(int argc, char *argv[])
{
	int serial_fd, n;
	char *command = nullptr;
	char *value = nullptr;
	char *device = nullptr;
	char newLine = '\n';

	if (argc >= 3 && argc <= 4) {
		device = argv[1];
		command = argv[2];
	} else {
		usage(argv);
	}

	size_t bufferReceiveSize = 1024; // 1KB
	uint8_t *bufferReceive = (uint8_t *) malloc(bufferReceiveSize);
	uint8_t *bufferReceive_p = bufferReceive;

	serial_fd = serial_open(device, B115200);
	if (serial_fd == -1) {
		std::cerr << "error: can't opening the serial device: " << device << std::endl;
		perror("OPEN");
		exit(0);
	}
	debug_printf("SERIAL OPEN:%s\n", device);

	switch (command[0]) {
	case 'C':
		serial_write(serial_fd, command, strlen(command));
		serial_write(serial_fd, &newLine, 1);
		debug_printf("<- %s\n", command);
		break;
	case 'S':
		if (argc == 4) {
			value = argv[3];
		} else {
			usage(argv);
		}

		debug_printf("value len:  %lu\n", strlen(value));

		serial_write(serial_fd, command, strlen(command));
		serial_write(serial_fd, &newLine, 1);
		debug_printf("<- %s\n", command);

		serial_write(serial_fd, value, strlen(value));
		serial_write(serial_fd, &newLine, 1);
		debug_printf("<- %s\n", value);
		break;
	case 'G':
		serial_write(serial_fd, command, strlen(command));
		serial_write(serial_fd, &newLine, 1);
		debug_printf("<- %s\n", command);

		do {
			n = serial_read(serial_fd, bufferReceive_p, (bufferReceiveSize) - (bufferReceive_p - bufferReceive), 1000 * 500);
			debug_printf("-> %d\n", n);

			if (n)
				bufferReceive_p += n;
			else
				break;

			if (*(bufferReceive_p - 1) == '\n')
				break;

			if (bufferReceive_p == bufferReceive + bufferReceiveSize) {
				bufferReceiveSize <<= 1;
				debug_printf("expanding buffer to %lu\n", bufferReceiveSize);

				if (bufferReceiveSize >= maxBufferSize) {
					std::cerr << "error: buffer size exceeded" << std::endl;
					break;
				}

				bufferReceive = (uint8_t*) realloc((void *) bufferReceive, bufferReceiveSize);
				bufferReceive_p = bufferReceive + bufferReceiveSize / 2;
			}
		} while(bufferReceive_p < bufferReceive + bufferReceiveSize);

// #if !b64_output
// 		// decode base64
// 		{
// 			uint8_t *bufferDecode = (uint8_t*) malloc(bufferReceiveSize);

// 			b64_decode(bufferReceive, (bufferReceive_p - bufferReceive), bufferDecode);
// 			fputs((const char *)bufferDecode, stdout);

// 			free(bufferDecode);
// 		}
// #else
		fputs((const char *)bufferReceive, stdout);
// #endif
		break;
	default:
		std::cerr << "error: unknown command: " << command << std::endl;
		exit(0);
		break;
	}

	serial_close(serial_fd);
	free(bufferReceive);	
	return 0;
}

int serial_open(char *serial_name, speed_t baud)
{
	int fd;

	fd = open(serial_name, O_RDWR | O_NOCTTY);

	struct termios tty;
	struct termios tty_old;
	memset (&tty, 0, sizeof(tty));

	/* Error Handling */
	if (tcgetattr (fd, &tty) != 0) {
	   std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
	   return -1;
	}

	/* Save old tty parameters */
	tty_old = tty;

	/* Set Baud Rate */
	cfsetospeed (&tty, baud);
	cfsetispeed (&tty, baud);

	/* Setting other Port Stuff */
	tty.c_cflag     &=  ~PARENB;            // Make 8n1
	tty.c_cflag     &=  ~CSTOPB;
	tty.c_cflag     &=  ~CSIZE;
	tty.c_cflag     |=  CS8;

	tty.c_cflag     &=  ~CRTSCTS;           // no flow control
	tty.c_cc[VMIN]   =  1;                  // read doesn't block
	tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
	tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

	/* Make raw */
	cfmakeraw(&tty);

	/* Flush Port, then applies attributes */
	tcflush(fd, TCIFLUSH);
	if ( tcsetattr (fd, TCSANOW, &tty) != 0) {
	   std::cerr << "Error " << errno << " from tcsetattr" << std::endl;
	   return -1;
	}

	return fd;
}

void serial_write(int serial_fd, char *data, int size)
{
	// since the receiver could have only a small buffer, send data slow
	for (int i = 0; i < size;) {
		if (size - i >= chunk) {
			write(serial_fd, data, chunk);
			i += chunk,
			data += chunk;
		} else {
			write(serial_fd, data, size - i);
			return;
		}

		debug_printf("send %d\n", i);

		usleep(sleepTime * 1000);
	}
}

int serial_read(int serial_fd, uint8_t *data, int size, int timeout_usec)
{
	fd_set fds;
	struct timeval timeout;
	int count=0;
	int ret;
	int n = 0;

	do {
		FD_ZERO(&fds);
		FD_SET(serial_fd, &fds);

		timeout.tv_sec = 0;
		timeout.tv_usec = timeout_usec;

		ret = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);

		if (ret == 1) {
			n = read(serial_fd, &data[count], size - count);
			count += n;
		}
		debug_printf(">> %d (%d/%d)\t", n, count, size);
		debug_printf(">> %s\n", data);
	} while (count < size && ret == 1);

	return count;
}

void serial_close(int fd)
{
	close(fd);
}

void usage(char *argv[])
{
	std::cout << "Usage: " << argv[0] << " device command [base64]" << std::endl;
	std::cout << "  Sn base64:  set bank n to base64" << std::endl;
	std::cout << "  Gn:         get bank n" << std::endl;
	std::cout << "  Cn:         clear bank n" << std::endl;
	exit(0);
}
