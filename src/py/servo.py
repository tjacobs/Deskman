#!/usr/bin/env python
import os
import time
import sys, tty, termios
from scservo_sdk import *

# Servo limits
SCS1_MINIMUM_POSITION_VALUE = 0
SCS1_MAXIMUM_POSITION_VALUE = 2000
SCS2_MINIMUM_POSITION_VALUE = 1400
SCS2_MAXIMUM_POSITION_VALUE = 1900
SCS_MOVING_SPEED            = 400
SCS_MOVING_ACC              = 100

# Settings
SCS1_ID                     = 1
SCS2_ID                     = 2
BAUDRATE                    = 115200        
DEVICENAME                  = '/dev/ttyAMA0'

# Control table address
ADDR_SCS_TORQUE_ENABLE      = 40
ADDR_STS_GOAL_ACC           = 41
ADDR_STS_GOAL_POSITION      = 42
ADDR_STS_GOAL_SPEED         = 46
ADDR_STS_PRESENT_POSITION   = 56

# Servo control
portHandler = None
packetHandler = None
groupSyncWrite = None

def setup_servos():
    global portHandler, packetHandler, groupSyncWrite
    
    # Initialize PortHandler instance
    portHandler = PortHandler(DEVICENAME)

    # Initialize PacketHandler instance
    packetHandler = PacketHandler(0)

    # Initialize GroupSyncWrite instance
    groupSyncWrite = GroupSyncWrite(portHandler, packetHandler, ADDR_STS_GOAL_POSITION, 2)

    # Open port
    if not portHandler.openPort():
        print("Failed to open the port")
        return False

    # Set port baudrate
    if not portHandler.setBaudRate(BAUDRATE):
        print("Failed to change the baudrate")
        return False

    # Configure servo 1
    scs_comm_result, scs_error = packetHandler.write1ByteTxRx(portHandler, SCS1_ID, ADDR_STS_GOAL_ACC, SCS_MOVING_ACC)
    if scs_comm_result != COMM_SUCCESS:
        print("%s" % packetHandler.getTxRxResult(scs_comm_result))
        return False
    elif scs_error != 0:
        print("%s" % packetHandler.getRxPacketError(scs_error))
        return False
    scs_comm_result, scs_error = packetHandler.write2ByteTxRx(portHandler, SCS1_ID, ADDR_STS_GOAL_SPEED, SCS_MOVING_SPEED)
    if scs_comm_result != COMM_SUCCESS:
        print("%s" % packetHandler.getTxRxResult(scs_comm_result))
        return False
    elif scs_error != 0:
        print("%s" % packetHandler.getRxPacketError(scs_error))
        return False

    # Configure servo 2
    scs_comm_result, scs_error = packetHandler.write1ByteTxRx(portHandler, SCS2_ID, ADDR_STS_GOAL_ACC, SCS_MOVING_ACC)
    if scs_comm_result != COMM_SUCCESS:
        print("%s" % packetHandler.getTxRxResult(scs_comm_result))
        return False
    elif scs_error != 0:
        print("%s" % packetHandler.getRxPacketError(scs_error))
        return False
    scs_comm_result, scs_error = packetHandler.write2ByteTxRx(portHandler, SCS2_ID, ADDR_STS_GOAL_SPEED, SCS_MOVING_SPEED)
    if scs_comm_result != COMM_SUCCESS:
        print("%s" % packetHandler.getTxRxResult(scs_comm_result))
        return False
    elif scs_error != 0:
        print("%s" % packetHandler.getRxPacketError(scs_error))
        return False

    return True

def move_head(x, y):
    """
    Move the head to the specified x, y position.
    x and y should be values between 0 and 1, where:
    0 = minimum position
    1 = maximum position
    """
    if portHandler is None or packetHandler is None or groupSyncWrite is None:
        print("Servos not initialized. Call setup_servos() first.")
        return False

    # Convert normalized coordinates to servo positions
    x_pos = int(SCS1_MINIMUM_POSITION_VALUE + x * (SCS1_MAXIMUM_POSITION_VALUE - SCS1_MINIMUM_POSITION_VALUE))
    y_pos = int(SCS2_MINIMUM_POSITION_VALUE + y * (SCS2_MAXIMUM_POSITION_VALUE - SCS2_MINIMUM_POSITION_VALUE))

    # Clamp values to valid range
    x_pos = max(SCS1_MINIMUM_POSITION_VALUE, min(x_pos, SCS1_MAXIMUM_POSITION_VALUE))
    y_pos = max(SCS2_MINIMUM_POSITION_VALUE, min(y_pos, SCS2_MAXIMUM_POSITION_VALUE))

    # Clear previous parameters
    groupSyncWrite.clearParam()

    # Add servo 1 (x-axis) position
    param_goal_position = [SCS_LOBYTE(x_pos), SCS_HIBYTE(x_pos)]
    if not groupSyncWrite.addParam(SCS1_ID, param_goal_position):
        print("[ID:%03d] groupSyncWrite addparam failed" % SCS1_ID)
        return False

    # Add servo 2 (y-axis) position
    param_goal_position = [SCS_LOBYTE(y_pos), SCS_HIBYTE(y_pos)]
    if not groupSyncWrite.addParam(SCS2_ID, param_goal_position):
        print("[ID:%03d] groupSyncWrite addparam failed" % SCS2_ID)
        return False

    # Syncwrite goal position
    scs_comm_result = groupSyncWrite.txPacket()
    if scs_comm_result != COMM_SUCCESS:
        print("%s" % packetHandler.getTxRxResult(scs_comm_result))
        return False

    return True

def cleanup():
    """Clean up resources"""
    if portHandler:
        portHandler.closePort()

if __name__ == "__main__":
    # Example usage
    if setup_servos():
        try:
            # Move to center up position
            move_head(0.5, 0.6)
            time.sleep(2)
            
            # Move to top-right
            move_head(1.0, 1.0)
            time.sleep(8)

            # Move to top-left
            move_head(0.0, 1.0)
            time.sleep(8)
            
            # Move to bottom-left
            move_head(0.0, 0.0)
            time.sleep(8)
	    
	    # Move back to center
            move_head(0.5, 0.5)
        finally:
            cleanup()
