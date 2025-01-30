#include "servos.h"
#include "../servos/SCSerial.h"
#include "../servos/SMS_STS.h"

std::string port_name = "/dev/ttyAMA0";

SMS_STS st;
SerialPort serial(port_name);

int open_servos() {
    // Open serial
    if (!serial.openPort()) return 1;
    st.pSerial = &serial;
    return 0;
}

void move_servos(int &x, int &y) {
    // Check if open
    if (!st.pSerial) {
       printf("Not moving\n");
       return;
    }

    // Limit
    int min_x = 0;
    int min_y = 1400;
    int max_x = 2000;
    int max_y = 1900;
    if (x < min_x) x = min_x;
    if (y < min_y) y = min_y;
    if (x > max_x) x = max_x;
    if (y > max_y) y = max_y;

    // Move
    st.WritePosEx(1, x, 800, 10);
    st.WritePosEx(2, y, 800, 10);
    int p1 = st.ReadPos(1);
    //printf("Position: %d\n", p1);
}
