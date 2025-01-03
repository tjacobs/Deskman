#include "SCSerial.h"
#include "SMS_STS.h"

int main() {
    // Open serial
    std::string port_name = "/dev/ttyAMA0";
    SerialPort serial(port_name);
    if (!serial.openPort()) return 1;
    SMS_STS st;
    st.pSerial = &serial;

    int x = 0;
    int y = 1500;

    do {
        // Update
        x += 10;
        y += 10;
        if (x > 2000) x = 0;
        if (y > 2000) y = 1500;

        // Move
        st.WritePosEx(1, x, 300, 10);
        st.WritePosEx(2, y, 300, 10);

        // Read
        int pos1 = st.ReadPos(1);
	printf("Pos1: %d\n", pos1);

        // Sleep
        usleep(10000);
    } while (true);

    return 0;
}


