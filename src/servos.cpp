#include "servos.h"
#include "../servos/SCSerial.h"
#include "../servos/SMS_STS.h"

SMS_STS st;

int open_servos() {
	// Open serial
    std::string port_name = "/dev/ttyAMA0";
    SerialPort serial(port_name);
    if (!serial.openPort()) return 1;
	st.pSerial = &serial;
    return 0;
}

void move_servos(int x, int y) {
    // Check if open
    if (!st.pSerial) return;

    // Limit
    int max_x = 2000;
    int max_y = 2000;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > max_x) x = max_x;
    if (y > max_y) y = max_y;

    // Move
	st.WritePosEx(1, x, 300, 10);
	st.WritePosEx(2, y, 300, 10);
}
