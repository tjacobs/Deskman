import cv2
import subprocess
import numpy as np
import mediapipe as mp
import signal
from servo import setup_servos, move_head, cleanup

# Setup MediaPipe
mp_face_detection = mp.solutions.face_detection
mp_drawing = mp.solutions.drawing_utils

# Damping parameters
dead_zone = 0.01  # Ignore movements smaller than this
smoothing_factor = 0.1  # How much to smooth the movement
max_movement = 0.5  # Maximum movement per frame
last_x = 0.5  # Last x position
last_y = 0.5  # Last y position

def damp_movement(target_x, target_y):
    """Apply damping to the movement values to reduce oscillation"""
    global last_x, last_y
    
    # Calculate distance
    dx = target_x - last_x
    dy = target_y - last_y
    
    # Apply dead zone
    if abs(dx) < dead_zone:
        dx = 0
    if abs(dy) < dead_zone:
        dy = 0
        
    # Limit maximum movement
    dx = max(min(dx, max_movement), -max_movement)
    dy = max(min(dy, max_movement), -max_movement)
    
    # Apply smoothing
    new_x = last_x + dx * smoothing_factor
    new_y = last_y + dy * smoothing_factor
    
    # Update last positions
    last_x = new_x
    last_y = new_y
    
    return new_x, new_y

# libcamera command
libcamera_cmd = [
    "libcamera-vid",
    "--width", "640",
    "--height", "480",
    "--framerate", "30",
    "--codec", "mjpeg",
    "--timeout", "0",
    "--nopreview",
    "-o", "-"
]

# ffmpeg command
ffmpeg_cmd = [
    "ffmpeg",
    "-loglevel", "error",
    "-f", "mjpeg",
    "-i", "pipe:0",
    "-pix_fmt", "bgr24",
    "-f", "rawvideo",
    "pipe:1"
]

# Launch libcamera and ffmpeg
libcamera_proc = subprocess.Popen(libcamera_cmd, stdout=subprocess.PIPE)
ffmpeg_proc = subprocess.Popen(ffmpeg_cmd, stdin=libcamera_proc.stdout, stdout=subprocess.PIPE)
libcamera_proc.stdout.close()  # let ffmpeg own this pipe

# Sizes
width, height = 640, 480
frame_size = width * height * 3

# Initialize servos
if not setup_servos():
    print("Failed to initialize servos")
    exit(1)

# Detect faces
with mp_face_detection.FaceDetection(model_selection=0, min_detection_confidence=0.5) as face_detection:
    try:
        while True:
            raw_frame = ffmpeg_proc.stdout.read(frame_size)
            if not raw_frame or len(raw_frame) != frame_size:
                break

            # Make the frame writeable
            frame = np.frombuffer(raw_frame, dtype=np.uint8).reshape((height, width, 3)).copy()

            # Face detection
            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            results = face_detection.process(rgb)
            if results.detections:
                # Find the largest face
                l_face = None
                l_area = 0
                for detection in results.detections:
                    # Get face bounding box
                    bbox = detection.location_data.relative_bounding_box
                    area = bbox.width * bbox.height
                    if area > l_area:
                        l_area = area
                        l_face = detection
                if l_face:
                    # Get face center
                    bbox = l_face.location_data.relative_bounding_box
                    x_center = bbox.xmin + bbox.width/2
                    y_center = bbox.ymin + bbox.height/2
                    
                    # Apply damping and move head
                    x_pos, y_pos = damp_movement(x_center, 1.0-y_center)
                    move_head(x_pos, y_pos)
                    
                    # Draw detection
                    #mp_drawing.draw_detection(frame, l_face)
                    
                    # Print values
                    #print(f"X: {x_center:.2f} Y: {y_center:.2f}")

            cv2.imshow("Face Detection", frame)
            if cv2.waitKey(1) & 0xFF == 27:
                break
    finally:
        print("Shutting down cleanly...")
        cleanup()  # Cleanup servos
        
        cv2.destroyAllWindows()

        # Cleanup subprocesses
        try: ffmpeg_proc.stdout.close()
        except: pass
        try: ffmpeg_proc.stdin.close()
        except: pass
        ffmpeg_proc.terminate()
        try: ffmpeg_proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            ffmpeg_proc.kill()

        libcamera_proc.terminate()
        try: libcamera_proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            libcamera_proc.kill()

