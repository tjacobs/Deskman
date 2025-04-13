#!/usr/bin/env python
#
# *********     Sync Write Example      *********
#
#
# Available SCServo model on this example : All models using Protocol SCS
# This example is tested with a SCServo(STS/SMS/SCS), and an URT
# Be sure that SCServo(STS/SMS/SCS) properties are already set as %% ID : 1 / Baudnum : 6 (Baudrate : 1000000)
#

import os

if os.name == 'nt':
    import msvcrt
    def getch():
        return msvcrt.getch().decode()
else:
    import sys, tty, termios
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    def getch():
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

from scservo_sdk import *                    # Uses SCServo SDK library

# Control table address
ADDR_SCS_TORQUE_ENABLE     = 40
ADDR_STS_GOAL_ACC          = 41
ADDR_STS_GOAL_POSITION     = 42
ADDR_STS_GOAL_SPEED        = 46
ADDR_STS_PRESENT_POSITION  = 56


# Default setting
SCS1_ID                     = 1                 # SCServo#1 ID : 1
SCS2_ID                     = 2                 # SCServo#1 ID : 2
BAUDRATE                    = 115200        
DEVICENAME                  = '/dev/ttyAMA0'    # Check which port is being used on your controller
                                                # ex) Windows: "COM1"   Linux: "/dev/ttyUSB0" Mac: "/dev/tty.usbserial-*"

SCS_MINIMUM_POSITION_VALUE  = 100               # SCServo will rotate between this value
SCS_MAXIMUM_POSITION_VALUE  = 4000              # and this value (note that the SCServo would not move when the position value is out of movable range. Check e-manual about the range of the SCServo you use.)
SCS_MOVING_STATUS_THRESHOLD = 20                # SCServo moving status threshold
SCS_MOVING_SPEED            = 255                 # SCServo moving speed
SCS_MOVING_ACC              = 255                 # SCServo moving acc
protocol_end                = 0                 # SCServo bit end(STS/SMS=0, SCS=1)

index = 0
scs_goal_position = [SCS_MINIMUM_POSITION_VALUE, SCS_MAXIMUM_POSITION_VALUE]         # Goal position


# Initialize PortHandler instance
# Set the port path
# Get methods and members of PortHandlerLinux or PortHandlerWindows
portHandler = PortHandler(DEVICENAME)

# Initialize PacketHandler instance
# Get methods and members of Protocol
packetHandler = PacketHandler(protocol_end)

# Initialize GroupSyncWrite instance
groupSyncWrite = GroupSyncWrite(portHandler, packetHandler, ADDR_STS_GOAL_POSITION, 2)

# Open port
if portHandler.openPort():
    print("")
else:
    print("Failed to open the port")
    print("Press any key to terminate...")
    getch()
    quit()


# Set port baudrate
if portHandler.setBaudRate(BAUDRATE):
    print("")
else:
    print("Failed to change the baudrate")
    print("Press any key to terminate...")
    getch()
    quit()

# SCServo#1 acc
# for i in range(0, 256):
#     scs_comm_result, scs_error = packetHandler.write1ByteTxRx(portHandler, i, ADDR_STS_GOAL_ACC, SCS_MOVING_ACC)
#     # print(i)

#     if scs_comm_result == COMM_SUCCESS:
#         print(i)
#     #     # pass
#     #     print("%s" % packetHandler.getTxRxResult(scs_comm_result))
#     # elif scs_error != 0:

#     #     print("%s" % packetHandler.getRxPacketError(scs_error))

#     time.sleep(0.01)


scs_comm_result, scs_error = packetHandler.write1ByteTxRx(portHandler, SCS1_ID, ADDR_STS_GOAL_ACC, SCS_MOVING_ACC)
if scs_comm_result != COMM_SUCCESS:
    print("%s" % packetHandler.getTxRxResult(scs_comm_result))
elif scs_error != 0:
    print("%s" % packetHandler.getRxPacketError(scs_error))


# SCServo#1 speed
scs_comm_result, scs_error = packetHandler.write2ByteTxRx(portHandler, SCS1_ID, ADDR_STS_GOAL_SPEED, SCS_MOVING_SPEED)
if scs_comm_result != COMM_SUCCESS:
    print("%s" % packetHandler.getTxRxResult(scs_comm_result))
elif scs_error != 0:
    print("%s" % packetHandler.getRxPacketError(scs_error))

# SCServo#1 torque
scs_comm_result, scs_error = packetHandler.write1ByteTxRx(portHandler, SCS1_ID, ADDR_SCS_TORQUE_ENABLE, 0)
if scs_comm_result != COMM_SUCCESS:
    print("%s" % packetHandler.getTxRxResult(scs_comm_result))
elif scs_error != 0:
    print("%s" % packetHandler.getRxPacketError(scs_error))


# Allocate goal position value into byte array
param_goal_position = [SCS_LOBYTE(scs_goal_position[index]), SCS_HIBYTE(scs_goal_position[index])]

# Add SCServo#1 goal position value to the Syncwrite parameter storage
scs_addparam_result = groupSyncWrite.addParam(SCS1_ID, param_goal_position)
if scs_addparam_result != True:
    print("[ID:%03d] groupSyncWrite addparam failed" % SCS1_ID)
    quit()

# Syncwrite goal position
scs_comm_result = groupSyncWrite.txPacket()
if scs_comm_result != COMM_SUCCESS:
    print("%s" % packetHandler.getTxRxResult(scs_comm_result))

# Clear syncwrite parameter storage
groupSyncWrite.clearParam()
