#include <stdio.h>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#ifdef __linux__
#include <linux/serial.h>
#endif
#include <sys/ioctl.h>

class SerialPort {
private:
    int serial_fd;
    std::string port_name;

public:
    SerialPort(const std::string &port) : port_name(port), serial_fd(-1) {}

    ~SerialPort() {
        if (serial_fd != -1) {
            close(serial_fd);
        }
    }

    bool configurePort(int serial_fd, int baud_rate) {
        // Get current serial port settings
        struct termios options;
        if (tcgetattr(serial_fd, &options) != 0) {
            std::cerr << "Failed to get serial port attributes: " << strerror(errno) << std::endl;
            return false;
        }

	// Set baud rate
	if (baud_rate == 115200) {
	    if (cfsetspeed(&options, B115200) != 0) {
	        std::cerr << "Failed to set BAUD_RATE" << std::endl;
                return -1;
            }
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
        options.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow
        options.c_oflag &= ~OPOST;

        // Apply the initial settings
        if (tcsetattr(serial_fd, TCSANOW, &options) != 0) {
            std::cerr << "Failed to set serial port attributes: " << strerror(errno) << std::endl;
            return false;
        }

#ifdef __linux__NO
        // Custom baud rate configuration
        struct serial_struct serial;
        if (ioctl(serial_fd, TIOCGSERIAL, &serial) != 0) {
            std::cerr << "Failed to get serial struct: " << strerror(errno) << std::endl;
            return false;
        }
        serial.flags &= ~ASYNC_SPD_MASK;
        serial.flags |= ASYNC_SPD_CUST;
        serial.custom_divisor = serial.baud_base / baud_rate;
        if (serial.custom_divisor == 0) {
            std::cerr << "Invalid custom divisor for baud rate " << baud_rate << std::endl;
            return false;
        }
        if (ioctl(serial_fd, TIOCSSERIAL, &serial) != 0) {
            std::cerr << "Failed to set serial struct: " << strerror(errno) << std::endl;
            return false;
        }
#else
//#warning Did not set baud rate!
#endif
        return true;
    }

    bool openPort() {
        serial_fd = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
        if (serial_fd == -1) {
            std::cerr << "Failed to open serial port " << port_name << std::endl;
            return false;
        }

        int baud_rate = 115200; //1000000;
        if (configurePort(serial_fd, baud_rate)) {
            std::cout << "Serial port configured successfully with baud rate: " << baud_rate << std::endl;
        } else {
            std::cerr << "Failed to configure serial port." << std::endl;
            close(serial_fd);
            return 1;
        }

        return true;
    }

    int readIn() {
        // Check if serial port is valid
        if (serial_fd == -1) {
            return -1;
        }

        // Use select to check if data is available
        fd_set read_fds;
        struct timeval timeout;

        FD_ZERO(&read_fds);
        FD_SET(serial_fd, &read_fds);

        // Set timeout to 0 for non-blocking check
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        int result = select(serial_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (result > 0 && FD_ISSET(serial_fd, &read_fds)) {
            unsigned char byte;
            int bytes_read = read(serial_fd, &byte, 1);

            if (bytes_read == 1) {
                return static_cast<int>(byte);
            }
        }

        // No data available or an error occurred
        return -1;
    }

    int writeOut(unsigned char *nDat, int nLen) {
        //printf("Writing: %d bytes.\n", nLen);
        std::string data(reinterpret_cast<char*>(nDat), nLen);
        writeData(data);
        return nLen;
    }

    bool writeData(const std::string &data) {
        if (serial_fd == -1) {
            std::cerr << "Serial port not open." << std::endl;
            return false;
        }

        int bytes_written = write(serial_fd, data.c_str(), data.length());
        if (bytes_written < 0) {
            std::cerr << "Failed to write to serial port." << std::endl;
            return false;
        }

        return true;
    }

    std::string readData(size_t max_length = 256) {
        if (serial_fd == -1) {
            std::cerr << "Serial port not open." << std::endl;
            return "";
        }

        char buffer[max_length];
        memset(buffer, 0, max_length);

        int bytes_read = read(serial_fd, buffer, max_length - 1);
        if (bytes_read < 0) {
            std::cerr << "Failed to read from serial port." << std::endl;
            return "";
        }

        return std::string(buffer, bytes_read);
    }
};
