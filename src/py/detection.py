import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib
import os
import numpy as np
import cv2
import hailo
from servo import setup_servos, move_head, cleanup

from hailo_apps_infra.hailo_rpi_common import (
    get_caps_from_pad,
    get_numpy_from_buffer,
    app_callback_class,
)
from hailo_apps_infra.detection_pipeline import GStreamerDetectionApp

# -----------------------------------------------------------------------------------------------
# User-defined class to be used in the callback function
# -----------------------------------------------------------------------------------------------
# Inheritance from the app_callback_class
class user_app_callback_class(app_callback_class):
    def __init__(self):
        super().__init__()
        self.new_variable = 42  # New variable example
        # Initialize servos
        if not setup_servos():
            print("Failed to initialize servos")
            self.servos_initialized = False
        else:
            self.servos_initialized = True
        
        # Damping parameters
        self.dead_zone = 0.1  # Ignore movements smaller than this
        self.smoothing_factor = 0.3  # How much to smooth the movement (0-1, higher = more smoothing)
        self.max_movement = 0.1  # Maximum movement per frame
        self.last_x = 0.5  # Last x position
        self.last_y = 0.5  # Last y position

    def new_function(self):  # New function example
        return "The meaning of life is: "

    def __del__(self):
        if self.servos_initialized:
            cleanup()

    def damp_movement(self, x_move, y_move):
        """Apply damping to the movement values to reduce oscillation"""
        # Convert to absolute positions
        target_x = 0.5 + x_move
        target_y = 0.5 + y_move
        
        # Calculate movement distance
        dx = target_x - self.last_x
        dy = target_y - self.last_y
        
        # Apply dead zone
        if abs(dx) < self.dead_zone:
            dx = 0
        if abs(dy) < self.dead_zone:
            dy = 0
            
        # Limit maximum movement
        dx = max(min(dx, self.max_movement), -self.max_movement)
        dy = max(min(dy, self.max_movement), -self.max_movement)
        
        # Apply smoothing
        new_x = self.last_x + dx * self.smoothing_factor
        new_y = self.last_y + dy * self.smoothing_factor
        
        # Update last positions
        self.last_x = new_x
        self.last_y = new_y
        
        return new_x, new_y

# -----------------------------------------------------------------------------------------------
# User-defined callback function
# -----------------------------------------------------------------------------------------------

# This is the callback function that will be called when data is available from the pipeline
def app_callback(pad, info, user_data):
    # Get the GstBuffer from the probe info
    buffer = info.get_buffer()
    # Check if the buffer is valid
    if buffer is None:
        return Gst.PadProbeReturn.OK

    # Using the user_data to count the number of frames
    user_data.increment()
    string_to_print = f"Frame count: {user_data.get_count()}\n"

    # Get the caps from the pad
    format, width, height = get_caps_from_pad(pad)

    # If the user_data.use_frame is set to True, we can get the video frame from the buffer
    frame = None
    if user_data.use_frame and format is not None and width is not None and height is not None:
        # Get video frame
        frame = get_numpy_from_buffer(buffer, format, width, height)

    # Get the detections from the buffer
    roi = hailo.get_roi_from_buffer(buffer)
    detections = roi.get_objects_typed(hailo.HAILO_DETECTION)

    # Parse the detections
    detection_count = 0
    for detection in detections:
        label = detection.get_label()
        bbox = detection.get_bbox()
        confidence = detection.get_confidence()
        if label == "person":
            # Get track ID
            track_id = 0
            track = detection.get_objects_typed(hailo.HAILO_UNIQUE_ID)
            x = bbox.xmin()
            y = bbox.ymin()
            w = bbox.width()
            h = bbox.height()
            x_center = x + w/2
            y_center = y + h/2
            x_move = 0.5 - x_center 
            y_move = 0.5 - y_center
            
            # Move the head based on the calculated values
            if user_data.servos_initialized:
                # Apply damping to the movement
                servo_x, servo_y = user_data.damp_movement(x_move, y_move)
                move_head(servo_x, servo_y)
            
            if len(track) == 1:
                track_id = track[0].get_id()
            string_to_print += (f"X: {x_move} Y: {y_move}\n")
            #string_to_print += (f"X: {x} Y: {y} W: {w} H: {h}\n")
            #string_to_print += (f"Detection: ID: {track_id} Label: {label} Confidence: {confidence:.2f}\n")
            detection_count += 1
    if user_data.use_frame:
        # Note: using imshow will not work here, as the callback function is not running in the main thread
        # Let's print the detection count to the frame
        cv2.putText(frame, f"Detections: {detection_count}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        # Example of how to use the new_variable and new_function from the user_data
        # Let's print the new_variable and the result of the new_function to the frame
        cv2.putText(frame, f"{user_data.new_function()} {user_data.new_variable}", (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        # Convert the frame to BGR
        frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
        user_data.set_frame(frame)

    print(string_to_print)
    return Gst.PadProbeReturn.OK

if __name__ == "__main__":
    # Create an instance of the user app callback class
    user_data = user_app_callback_class()
    app = GStreamerDetectionApp(app_callback, user_data)
    app.run()
