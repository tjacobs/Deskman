import cv2
import numpy
import subprocess
import mediapipe as mp
import servo

# MediaPipe
mp_face_detection = mp.solutions.face_detection
mp_drawing = mp.solutions.drawing_utils

# Try to move face to these positions
target_face_x = 0.55
target_face_y = 0.40
move_amount = 0.08
move_max = 0.001
last_x = 0.5
last_y = 0.5

def update_movement(face_x, face_y):
    global last_x, last_y

    # Calculate how much to move
    dx = (face_x - target_face_x) * move_amount
    dy = (face_y - target_face_y) * move_amount * 2

    # Calculate new values
    new_x = last_x + dx
    new_y = last_y + dy

    # Limit maximum movement
    dx = max(min(dx, move_max), -move_max)
    dy = max(min(dy, move_max), -move_max)

    print(f"dx: {dx:.4f} new_x: {new_x:.4f}")
    #print(f"dy: {dy:.4f} new_y: {new_y:.4f}")


    # Update last positions
    last_x = new_x
    last_y = new_y
    if last_x < 0: last_x = 0
    if last_y < 0: last_y = 0
    if last_x > 1: last_x = 1
    if last_y > 1: last_y = 1
   
    # Return new
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
libcamera_proc.stdout.close()

# Sizes
width, height = 640, 480
frame_size = width * height * 3

# Initialize servos
if not servo.setup_servos():
    print("Failed to initialize servos")
    exit(1)

# Detect faces
with mp_face_detection.FaceDetection(model_selection=0, min_detection_confidence=0.5) as face_detection:
    try:
        while True:
            # Read frame
            raw_frame = ffmpeg_proc.stdout.read(frame_size)
            if not raw_frame or len(raw_frame) != frame_size:
                break

            # Make the frame writeable
            frame = numpy.frombuffer(raw_frame, dtype=numpy.uint8).reshape((height, width, 3)).copy()

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

                # If found
                if l_face:
                    # Get face center
                    bbox = l_face.location_data.relative_bounding_box
                    x_c = bbox.xmin + bbox.width / 2
                    y_c = 1.0 - (bbox.ymin + bbox.height / 2)
                    
                    # Apply movement and move head
                    x_pos, y_pos = update_movement(x_c, y_c)
                    servo.move_head(x_pos, y_pos)
                    
                    # Draw detection
                    mp_drawing.draw_detection(frame, l_face)
                    
                    # Print values
                    print(f"X: {x_c:.2f} Y: {y_c:.2f}")

            # Show
            cv2.imshow("Face Detection", frame)

            # Check escape
            if cv2.waitKey(1) & 0xFF == 27: break
    finally:
        servo.cleanup()
        
        cv2.destroyAllWindows()

        # Cleanup subprocesses
        try: ffmpeg_proc.stdout.close()
        except: pass
        try: ffmpeg_proc.stdin.close()
        except: pass
        ffmpeg_proc.terminate()
        try: ffmpeg_proc.wait(timeout=2)
        except subprocess.TimeoutExpired: ffmpeg_proc.kill()
        libcamera_proc.terminate()
        try: libcamera_proc.wait(timeout=2)
        except subprocess.TimeoutExpired: libcamera_proc.kill()

