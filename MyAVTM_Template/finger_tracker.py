import cv2
import numpy as np
import time
import math
import socket
import sys
import os

import mediapipe as mp
from mediapipe.tasks.python import vision, BaseOptions

UDP_IP, UDP_PORT = "127.0.0.1", 9099
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

MODEL_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "hand_landmarker.task")
if not os.path.exists(MODEL_PATH):
    MODEL_PATH = "hand_landmarker.task"

# ---- Tunables ----
PINCH_ENTER = 0.25     # pinch_dist / hand_size below this -> start pinch
PINCH_EXIT = 0.40      # above this -> release pinch (hysteresis)
LOST_GRACE = 5         # frames to keep state if the hand briefly disappears
MIN_CUTOFF = 1.2       # lower = smoother when slow (more lag)
BETA = 8.0             # higher = more responsive when moving fast

# Hand validity checks (reject false positives like faces / lamps)
MIN_HAND_FRAC, MAX_HAND_FRAC = 0.08, 0.60   # palm length relative to frame height
MIN_HANDEDNESS = 0.80
BAD_FRAMES_RESET = 8   # consecutive junk frames before forcing a fresh detection


class OneEuro:
    """One Euro filter: adaptive low-pass for jittery tracking data."""
    def __init__(self, min_cutoff=1.0, beta=0.0, d_cutoff=1.0):
        self.min_cutoff, self.beta, self.d_cutoff = min_cutoff, beta, d_cutoff
        self.x_prev = None
        self.dx_prev = 0.0
        self.t_prev = None

    @staticmethod
    def _alpha(cutoff, dt):
        tau = 1.0 / (2 * math.pi * cutoff)
        return 1.0 / (1.0 + tau / dt)

    def reset(self):
        self.x_prev = None
        self.dx_prev = 0.0
        self.t_prev = None

    def __call__(self, x, t):
        if self.x_prev is None:
            self.x_prev, self.t_prev = x, t
            return x
        dt = max(t - self.t_prev, 1e-3)
        dx = (x - self.x_prev) / dt
        a_d = self._alpha(self.d_cutoff, dt)
        dx_hat = a_d * dx + (1 - a_d) * self.dx_prev
        cutoff = self.min_cutoff + self.beta * abs(dx_hat)
        a = self._alpha(cutoff, dt)
        x_hat = a * x + (1 - a) * self.x_prev
        self.x_prev, self.dx_prev, self.t_prev = x_hat, dx_hat, t
        return x_hat


def make_landmarker():
    opts = vision.HandLandmarkerOptions(
        base_options=BaseOptions(model_asset_path=MODEL_PATH),
        running_mode=vision.RunningMode.VIDEO,      # tracks across frames
        num_hands=1,
        min_hand_detection_confidence=0.7,
        min_hand_presence_confidence=0.7,
        min_tracking_confidence=0.6,
    )
    return vision.HandLandmarker.create_from_options(opts)


def looks_like_hand(pts, handedness, h):
    """Sanity-check a landmark set so random blobs are rejected."""
    if not handedness or handedness[0].score < MIN_HANDEDNESS:
        return False
    d = lambda i, j: math.hypot(pts[i][0] - pts[j][0], pts[i][1] - pts[j][1])
    palm_len = d(0, 9)
    palm_w = d(5, 17)
    if palm_len < 1:
        return False
    # hand must be a sane size for the frame
    if not (MIN_HAND_FRAC * h < palm_len < MAX_HAND_FRAC * h):
        return False
    # palm proportions: width/length is roughly 0.6-1.3 on a real hand
    if not (0.45 < palm_w / palm_len < 1.6):
        return False
    # fingers can't be absurdly longer than the palm
    for tip in (8, 12, 16, 20):
        if d(0, tip) > 3.2 * palm_len:
            return False
    return True


print(f"[FingerTracker] Loading MediaPipe model from {MODEL_PATH}...")
landmarker = make_landmarker()
print("[FingerTracker] MediaPipe HandLandmarker loaded successfully!")

HAND_CONNECTIONS = [
    (0, 1), (1, 2), (2, 3), (3, 4),
    (0, 5), (5, 6), (6, 7), (7, 8),
    (5, 9), (9, 10), (10, 11), (11, 12),
    (9, 13), (13, 14), (14, 15), (15, 16),
    (13, 17), (17, 18), (18, 19), (19, 20),
    (0, 17),
]

cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)
if not cap.isOpened():
    cap = cv2.VideoCapture(0)
if not cap.isOpened():
    print("[FingerTracker] ERROR: Could not open camera 0!")
    sys.exit(1)

cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"MJPG"))  # higher FPS on most webcams
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
cap.set(cv2.CAP_PROP_FPS, 30)
cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)                            # less latency

cv2.namedWindow("Finger Path Drawing", cv2.WINDOW_AUTOSIZE)

strokes = []            # list of strokes, each a list of (x, y) pixels
current_stroke = None
stable_pinch = False
lost_frames = 0
bad_frames = 0
last_norm = (0.5, 0.5)
last_dist = 1.0
filt_x = OneEuro(MIN_CUTOFF, BETA)
filt_y = OneEuro(MIN_CUTOFF, BETA)

fps_timer = time.time()
frame_count = 0
current_fps = 0.0
last_ts_ms = -1
start = time.perf_counter()

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
            time.sleep(0.005)
            continue

        frame = cv2.flip(frame, 1)
        h, w, _ = frame.shape

        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)

        # VIDEO mode needs strictly increasing timestamps
        t_now = time.perf_counter() - start
        ts_ms = int(t_now * 1000)
        if ts_ms <= last_ts_ms:
            ts_ms = last_ts_ms + 1
        last_ts_ms = ts_ms
        result = landmarker.detect_for_video(mp_image, ts_ms)

        detected = False
        norm_x, norm_y = last_norm
        pinch_ratio = last_dist
        px = py = None

        # Validate the detection
        hand_ok = False
        pts = None
        if result.hand_landmarks:
            lm = result.hand_landmarks[0]
            pts = [(int(p.x * w), int(p.y * h)) for p in lm]
            hand_ok = looks_like_hand(
                pts, result.handedness[0] if result.handedness else None, h
            )

        if hand_ok:
            bad_frames = 0
            detected = True
            lost_frames = 0

            for a, b in HAND_CONNECTIONS:
                cv2.line(frame, pts[a], pts[b], (0, 220, 100), 2)
            for pt in pts:
                cv2.circle(frame, pt, 4, (255, 200, 0), -1)

            # Pixel-space distances (fixes aspect-ratio distortion)
            def pdist(i, j):
                return math.hypot(pts[i][0] - pts[j][0], pts[i][1] - pts[j][1])

            # Hand size: wrist->middle MCP and palm width, averaged
            hand_size = max(0.5 * (pdist(0, 9) + pdist(5, 17)), 1.0)
            pinch_ratio = pdist(4, 8) / hand_size   # scale-invariant

            # Hysteresis on the *distance*, not on frame counts
            if stable_pinch:
                if pinch_ratio > PINCH_EXIT:
                    stable_pinch = False
            else:
                if pinch_ratio < PINCH_ENTER:
                    stable_pinch = True

            # Always track the thumb/index midpoint -> no jump when pinching starts
            target_x = (lm[4].x + lm[8].x) * 0.5
            target_y = (lm[4].y + lm[8].y) * 0.5

            norm_x = min(max(filt_x(target_x, t_now), 0.0), 1.0)
            norm_y = min(max(filt_y(target_y, t_now), 0.0), 1.0)
            last_norm, last_dist = (norm_x, norm_y), pinch_ratio
            px, py = int(norm_x * w), int(norm_y * h)

        else:
            if result.hand_landmarks:
                # Something was "detected" but failed validation
                bad_frames += 1
                if bad_frames > BAD_FRAMES_RESET:
                    # tracker is stuck on junk -> force fresh detection
                    landmarker.close()
                    landmarker = make_landmarker()
                    start = time.perf_counter()
                    last_ts_ms = -1
                    bad_frames = 0
            else:
                bad_frames = 0

            lost_frames += 1
            if lost_frames <= LOST_GRACE:
                # brief dropout: hold last state so strokes don't break
                detected = True
                px, py = int(norm_x * w), int(norm_y * h)
            else:
                stable_pinch = False
                filt_x.reset()
                filt_y.reset()
                cv2.putText(frame, "NO HAND DETECTED", (30, 95),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 165, 255), 2)

        # Stroke management
        if detected and stable_pinch and px is not None:
            if current_stroke is None:
                current_stroke = []
                strokes.append(current_stroke)
            current_stroke.append((px, py))
        else:
            current_stroke = None

        # Cursor
        if detected and px is not None:
            if stable_pinch:
                cv2.circle(frame, (px, py), 12, (0, 0, 255), -1)
                cv2.circle(frame, (px, py), 20, (0, 0, 255), 2)
                cv2.putText(frame, "PINCH: DRAWING", (px + 25, py - 10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 0, 255), 2)
            else:
                cv2.circle(frame, (px, py), 10, (0, 255, 0), -1)
                cv2.circle(frame, (px, py), 18, (0, 255, 0), 2)
                cv2.putText(frame, "HOVER: PREVIEW", (px + 25, py - 10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 255, 0), 2)

        # Draw strokes (separate, not connected to each other)
        for s in strokes:
            if len(s) > 1:
                cv2.polylines(frame, [np.array(s, np.int32)], False, (0, 255, 255), 3)

        # UDP: "DETECTED,PINCH,X,Y,DIST\n"  (DIST is now pinch/hand-size ratio)
        msg = f"{int(detected)},{int(stable_pinch)},{norm_x:.5f},{norm_y:.5f},{pinch_ratio:.5f}\n"
        try:
            sock.sendto(msg.encode("ascii"), (UDP_IP, UDP_PORT))
        except Exception:
            pass

        # HUD
        cv2.rectangle(frame, (0, 0), (w, 65), (25, 25, 25), -1)
        cv2.putText(frame, "MEDIAPIPE FINGER TRACKING", (15, 25),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 240, 255), 2)
        cv2.putText(frame, "[Pinch]: Draw  |  [C / Enter]: Confirm & Drive  |  [X]: Clear  |  [L]: Loop",
                    (15, 52), cv2.FONT_HERSHEY_SIMPLEX, 0.45, (220, 220, 220), 1)

        frame_count += 1
        now = time.time()
        if now - fps_timer >= 1.0:
            current_fps = frame_count / (now - fps_timer)
            frame_count = 0
            fps_timer = now
        total_pts = sum(len(s) for s in strokes)
        cv2.putText(frame, f"FPS: {int(current_fps)}", (w - 120, 25),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 0), 2)
        cv2.putText(frame, f"Points: {total_pts}", (w - 140, 50),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 240, 255), 1)
        cv2.putText(frame, f"ratio: {pinch_ratio:.2f}", (w - 140, 75),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)

        cv2.imshow("Finger Path Drawing", frame)
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q') or key == 27:
            break
        elif key in (ord('c'), 13):
            sock.sendto(b"CMD:CONFIRM\n", (UDP_IP, UDP_PORT))
        elif key == ord('x'):
            strokes.clear()
            current_stroke = None
            sock.sendto(b"CMD:CLEAR\n", (UDP_IP, UDP_PORT))
        elif key == ord('l'):
            sock.sendto(b"CMD:LOOP\n", (UDP_IP, UDP_PORT))

finally:
    cap.release()
    cv2.destroyAllWindows()
    sock.close()
    landmarker.close()