//#include <linux/serial.h>
#include <sys/ioctl.h>

int open_servos();
void move_servos(int &x, int &y);
