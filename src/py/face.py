import cv2
import subprocess
import numpy as np
import mediapipe as mp
import signal

# Setup MediaPipe
mp_face_detection = mp.solutions.face_detection
mp_drawing = mp.solutions.drawing_utils

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

width, height = 640, 480
frame_size = width * height * 3

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
                for detection in results.detections:
                    mp_drawing.draw_detection(frame, detection)

            cv2.imshow("Face Detection", frame)
            if cv2.waitKey(1) & 0xFF == 27:
                break
    finally:
        print("Shutting down cleanly...")

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

