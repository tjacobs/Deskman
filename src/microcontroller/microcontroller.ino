// Set up servos:
// Program them from 1M baud to 115K baud
// And set their IDs
// And test moving and reading positions
#include <SCServo.h>
SMS_STS st;

void setup() {
  Serial.begin(115200);
  //#define S_RXD 18 // For waveshare ESP32
  //#define S_TXD 19 // 
  int currentBaud = 1000000; //115200;
  Serial1.begin(currentBaud, SERIAL_8N1); //, S_RXD, S_TXD);
  st.pSerial = &Serial1;
  delay(1000);

  // Change baud rate of servos from 1000000 to 115200
#if 0
  int ID_ChangeFrom = 1;
  int ID_Changeto   = 1;
  st.unLockEprom(ID_ChangeFrom);
//  st.writeByte(ID_ChangeFrom, SMS_STS_ID, ID_Changeto); // Change servo serial ID
  #define SMS_STS_1M    0
  #define SMS_STS_115200  4
  st.writeByte(ID_ChangeFrom, SMS_STS_BAUD_RATE, SMS_STS_115200);
  st.LockEprom(ID_Changeto);
#endif
}

// Test moving servos
int x = 0;
int y = 1500;
long t_start = millis();
void loop() {
  // Move
  long t = millis();
  if (t - t_start > 1000) {
    x += 100;
    y += 100;
    t_start = millis();
  }
  if (x > 1000) x = 0;
  if (y > 2048) y = 1500;

  // Proxy commands: Read in from USB serial
  int i = 0;
  unsigned char bytes[100];
  while (Serial.available()) {
    int b = Serial.read();
    if (b == -1) break;
    bytes[i] = b;
    i++;
  }
  bytes[i] = 0;

  // Write out to servo serial
  if (i > 0) {
    Serial1.write(bytes, i);
//    Serial.print("Got: ");
//    Serial.print((char*)bytes);
//    Serial.print("\n");
  }
  else {
    // Or write calculated positions
    st.WritePosEx(1, x, 500, 10);
    st.WritePosEx(2, y, 500, 10);
    st.WritePosEx(3, y, 500, 10);
  }

  // Read positions
  int pos1 = st.ReadPos(1);
  Serial.print("Pos1: ");
  Serial.println(pos1, DEC);
  int pos2 = st.ReadPos(2);
  Serial.print("Pos2: ");
  Serial.println(pos2, DEC);
  int pos3 = st.ReadPos(3);
  Serial.print("Pos3: ");
  Serial.println(pos3, DEC);

  // Sleep
  delay(10);
}

// Bonus commands
//  st.EnableTorque(1, true);
//  st.EnableTorque(2, true);
//  st.EnableTorque(3, true);
//  st.WheelMode(3);
//  st.WriteSpe(3, 0, 100);
//  st.WriteSpe(3, 200, 100);