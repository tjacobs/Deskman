#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define BAUD_RATE 1000000

int configureSerialPort(int fd) {
    struct termios options;

    // Get the current attributes of the serial port
    if (tcgetattr(fd, &options) != 0) {
        perror("Failed to get serial port attributes");
        return -1;
    }

    // Set the baud rates
    if (cfsetspeed(&options, BAUD_RATE) != 0) {
        perror("Failed to set baud rate");
        return -1;
    }

    // Configure 8N1
    options.c_cflag &= ~PARENB;  // No parity
    options.c_cflag &= ~CSTOPB;  // One stop bit
    options.c_cflag &= ~CSIZE;   // Clear data size bits
    options.c_cflag |= CS8;      // 8 data bits

    // Enable the receiver and set local mode
    options.c_cflag |= (CLOCAL | CREAD);

    // Disable hardware flow control
    options.c_cflag &= ~CRTSCTS;

    // Raw input/output mode
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control
    options.c_oflag &= ~OPOST;

    // Apply the new attributes
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        perror("Failed to set serial port attributes");
        return -1;
    }

    return 0;
}

int main() {
    int serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    if (serial_fd == -1) {
        perror("Failed to open serial port");
        return 1;
    }

    if (configureSerialPort(serial_fd) != 0) {
        close(serial_fd);
        return 1;
    }

    const char *test_message = "Hello, Raspberry Pi!";
    size_t message_length = strlen(test_message);

    // Write data to the serial port
    ssize_t bytes_written = write(serial_fd, test_message, message_length);
    if (bytes_written < 0) {
        perror("Failed to write to serial port");
        close(serial_fd);
        return 1;
    }

    printf("Written %zd bytes: %s\n", bytes_written, test_message);

    // Wait for data to be available and read it
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));

    printf("Waiting for response...\n");

    ssize_t bytes_read;
    do {
        bytes_read = read(serial_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0 && errno != EAGAIN) {
            perror("Failed to read from serial port");
            close(serial_fd);
            return 1;
        }
    } while (bytes_read <= 0); // Retry if no data is available yet

    printf("Read %zd bytes: %s\n", bytes_read, buffer);

    close(serial_fd);
    return 0;
}
