#include "SCSerial.h"
#include "SMS_STS.h"

int main() {
    // Open serial
    std::string port_name = "/dev/ttyACM0";
    SerialPort serial(port_name);
    if (!serial.openPort()) return 1;
    SMS_STS st;
    st.pSerial = &serial;

    // Positions
    int x = 0;
    int y = 1500;
    do {
        // Update
        x += 20;
        y += 10;
        if (x > 2000) x = 0;
        if (y > 1800) y = 1300;

        // Move
        st.WritePosEx(1, x, 300, 10);
        st.WritePosEx(2, y, 300, 10);
    
        // Read
        //int pos1 = st.ReadPos(1);
        //printf("Pos1: %d  (x=%d)\n", pos1, x);

        // Read message
        //char buffer[256];
        //memset(buffer, 0, sizeof(buffer));
        //printf("Waiting for response...\n");
        //ssize_t bytes_read;
        //std::string str = serial.readData();
        //printf("Read: %d\n", str.size());        
        //bytes_read = read(serial, buffer, sizeof(buffer) - 1);
        //if (bytes_read < 0 && errno != EAGAIN) {
            //perror("Failed to read from serial port");
            //close(serial);
            //return 1;
        //}
        //usleep(100000);
        //printf("Read %zd bytes: %s\n", bytes_read, buffer);    

        // Sleep
        usleep(10000);
    } while (true);

    return 0;
}
