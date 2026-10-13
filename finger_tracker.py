import cv2
import numpy as np
import time
import math
import socket
import sys
import os

import mediapipe as mp
from mediapipe.tasks.python import vision, BaseOptions

# UDP Socket to send tracking data to C++ game
UDP_IP = "127.0.0.1"
UDP_PORT = 9099
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Model path
MODEL_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "hand_landmarker.task")
if not os.path.exists(MODEL_PATH):
    MODEL_PATH = "hand_landmarker.task"

print(f"[FingerTracker] Loading MediaPipe model from {MODEL_PATH}...")
options = vision.HandLandmarkerOptions(
    base_options=BaseOptions(model_asset_path=MODEL_PATH),
    running_mode=vision.RunningMode.IMAGE,
    num_hands=1,
    min_hand_detection_confidence=0.6,
    min_hand_presence_confidence=0.6,
    min_tracking_confidence=0.6
)
landmarker = vision.HandLandmarker.create_from_options(options)
print("[FingerTracker] MediaPipe HandLandmarker loaded successfully!")

# Connections between hand landmarks (0 to 20)
HAND_CONNECTIONS = [
    (0, 1), (1, 2), (2, 3), (3, 4),        # Thumb
    (0, 5), (5, 6), (6, 7), (7, 8),        # Index
    (5, 9), (9, 10), (10, 11), (11, 12),   # Middle
    (9, 13), (13, 14), (14, 15), (15, 16), # Ring
    (13, 17), (17, 18), (18, 19), (19, 20),# Pinky
    (0, 17)                                # Palm base
]

cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)
if not cap.isOpened():
    cap = cv2.VideoCapture(0)

if not cap.isOpened():
    print("[FingerTracker] ERROR: Could not open camera 0!")
    sys.exit(1)

cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
cap.set(cv2.CAP_PROP_FPS, 30)

cv2.namedWindow("Finger Path Drawing", cv2.WINDOW_AUTOSIZE)

trail = []
prev_pinch = False
fps_timer = time.time()
frame_count = 0
current_fps = 0.0

# Temporal EMA smoothing
smoothed_x = 0.5
smoothed_y = 0.5
has_smooth = False
consecutive_pinch = 0
consecutive_open = 0
stable_pinch = False

print("[FingerTracker] Running! Showing window 'Finger Path Drawing'...")
print("  - Pinch thumb + index to DRAW")
print("  - Release to HOVER / PREVIEW")
print("  - Press [C] / [Enter] to CONFIRM and drive")
print("  - Press [X] to CLEAR")
print("  - Press [L] to toggle LOOP")

try:
    while True:
        ret, frame = cap.read()
        if not ret or frame is None:
            time.sleep(0.01)
            continue

        # Mirror frame horizontally so user moves naturally
        frame = cv2.flip(frame, 1)
        h, w, _ = frame.shape

        # Convert to RGB for MediaPipe
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)

        result = landmarker.detect(mp_image)

        detected = False
        raw_pinch = False
        norm_x = 0.5
        norm_y = 0.5
        pinch_dist = 1.0

        if result.hand_landmarks and len(result.hand_landmarks) > 0:
            landmarks = result.hand_landmarks[0]
            detected = True

            # Landmark coordinates in pixels
            pts = [(int(lm.x * w), int(lm.y * h)) for lm in landmarks]

            # Draw skeleton connections
            for p1, p2 in HAND_CONNECTIONS:
                cv2.line(frame, pts[p1], pts[p2], (0, 220, 100), 2)

            for pt in pts:
                cv2.circle(frame, pt, 4, (255, 200, 0), -1)

            # Landmark 4: Thumb tip, Landmark 8: Index fingertip
            thumb = landmarks[4]
            index = landmarks[8]

            # Euclidean distance in normalized coordinates
            dx = thumb.x - index.x
            dy = thumb.y - index.y
            dz = thumb.z - index.z
            pinch_dist = math.sqrt(dx * dx + dy * dy)

            # Adaptive pinch threshold (~5.5% of frame width/height)
            PINCH_THRESH = 0.065

            if pinch_dist < PINCH_THRESH:
                raw_pinch = True
                target_x = (thumb.x + index.x) * 0.5
                target_y = (thumb.y + index.y) * 0.5
            else:
                raw_pinch = False
                target_x = index.x
                target_y = index.y

            # Temporal hysteresis for pinch
            if raw_pinch:
                consecutive_pinch += 1
                consecutive_open = 0
                if consecutive_pinch >= 2:
                    stable_pinch = True
            else:
                consecutive_open += 1
                consecutive_pinch = 0
                if consecutive_open >= 2:
                    stable_pinch = False

            # EMA smoothing
            if not has_smooth:
                smoothed_x = target_x
                smoothed_y = target_y
                has_smooth = True
            else:
                alpha = 0.75
                smoothed_x = alpha * target_x + (1.0 - alpha) * smoothed_x
                smoothed_y = alpha * target_y + (1.0 - alpha) * smoothed_y

            norm_x = smoothed_x
            norm_y = smoothed_y

            # Pixel position for visualization
            px = int(norm_x * w)
            py = int(norm_y * h)

            if stable_pinch:
                trail.append((px, py))
                # Red pulsing cursor
                cv2.circle(frame, (px, py), 12, (0, 0, 255), -1)
                cv2.circle(frame, (px, py), 20, (0, 0, 255), 2)
                cv2.putText(frame, "PINCH: DRAWING", (px + 25, py - 10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 0, 255), 2)
            else:
                # Green hover cursor
                cv2.circle(frame, (px, py), 10, (0, 255, 0), -1)
                cv2.circle(frame, (px, py), 18, (0, 255, 0), 2)
                cv2.putText(frame, "HOVER: PREVIEW", (px + 25, py - 10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 255, 0), 2)
        else:
            detected = False
            stable_pinch = False
            has_smooth = False
            consecutive_pinch = 0
            cv2.putText(frame, "NO HAND DETECTED", (30, 95),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 165, 255), 2)

        # Draw recorded trail on camera feed
        if len(trail) > 1:
            for i in range(1, len(trail)):
                cv2.line(frame, trail[i - 1], trail[i], (0, 255, 255), 3)

        # Send UDP packet to C++ game: "DETECTED,PINCH,X,Y,DIST\n"
        det_int = 1 if detected else 0
        pinch_int = 1 if stable_pinch else 0
        msg = f"{det_int},{pinch_int},{norm_x:.5f},{norm_y:.5f},{pinch_dist:.5f}\n"
        try:
            sock.sendto(msg.encode("ascii"), (UDP_IP, UDP_PORT))
        except Exception:
            pass

        # Top banner
        cv2.rectangle(frame, (0, 0), (w, 65), (25, 25, 25), -1)
        cv2.putText(frame, "MEDIAPAI FINGER TRACKING (AI SKELETON)", (15, 25),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 240, 255), 2)
        cv2.putText(frame, "[Pinch]: Draw  |  [C / Enter]: Confirm & Drive  |  [X]: Clear  |  [L]: Loop",
                    (15, 52), cv2.FONT_HERSHEY_SIMPLEX, 0.45, (220, 220, 220), 1)

        # FPS
        frame_count += 1
        now = time.time()
        if now - fps_timer >= 1.0:
            current_fps = frame_count / (now - fps_timer)
            frame_count = 0
            fps_timer = now

        cv2.putText(frame, f"FPS: {int(current_fps)}", (w - 120, 25),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 0), 2)
        cv2.putText(frame, f"Points: {len(trail)}", (w - 140, 50),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 240, 255), 1)

        cv2.imshow("Finger Path Drawing", frame)
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q') or key == 27:
            break
        elif key == ord('c') or key == 13: # Enter or C
            sock.sendto(b"CMD:CONFIRM\n", (UDP_IP, UDP_PORT))
        elif key == ord('x'): # Clear
            trail.clear()
            sock.sendto(b"CMD:CLEAR\n", (UDP_IP, UDP_PORT))
        elif key == ord('l'): # Loop
            sock.sendto(b"CMD:LOOP\n", (UDP_IP, UDP_PORT))

finally:
    cap.release()
    cv2.destroyAllWindows()
    sock.close()
