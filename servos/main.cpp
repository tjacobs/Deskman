#include "SCSerial.h"
#include "SMS_STS.h"

int main() {
	// Open serial
    std::string port_name = "/dev/tty.usbmodem585A0081791";
    SerialPort serial(port_name);
    if (!serial.openPort()) return 1;

    // Open ST servo driver
	SMS_STS st;
	st.pSerial = &serial;

	// Move
	int x = 0;
	int y = 1500;
	st.WritePosEx(1, x, 500, 10);
	st.WritePosEx(2, y, 500, 10);

	int pos1 = st.ReadPos(1);
	printf("Pos1: %d\n", pos1);

    /*std::string message = "\n";
    if (serial.writeData(message)) {
        std::cout << "Message sent: " << message << std::endl;
    }

    std::string response = serial.readData();
    if (!response.empty()) {
        std::cout << "Message received: " << response << std::endl;
    }*/

	return 0;
}


