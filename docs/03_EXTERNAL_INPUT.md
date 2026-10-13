# Stage 3 --- Arduino Accelerometer Path Input

## Objective

Use the available Arduino Sensor Kit accelerometer as an alternative way
to create a path. The resulting path must use the same raw-sample and
canonical-path pipeline as the mouse editor. The path follower and car
physics must remain independent of the sensor hardware.

The first version should implement a practical
**tilt/gesture-to-cursor** mapping, not assume that raw accelerometer
data can reconstruct absolute pen position.

## Important sensor limitation

An accelerometer measures specific force/acceleration, including
gravity. It does not directly measure absolute position. Integrating
acceleration once estimates velocity; integrating twice estimates
position, but small bias and noise quickly accumulate into large drift.
Therefore, pure double integration is not an appropriate default for
drawing a stable path.

Use one of these practical mappings, in order of implementation
preference:

1.  **Tilt-to-cursor velocity:** tilting the device moves a virtual
    cursor across the drawing plane. The user starts recording, moves
    the cursor using tilt, and stops recording.
2.  **Tilt-to-cursor position:** map calibrated tilt angles or
    gravity-relative axes to cursor position in a bounded drawing area.
    This can be easier to reset but may feel like a joystick rather than
    drawing.
3.  **Gesture-based relative movement:** detect intentional directional
    gestures and move the virtual cursor by controlled increments. Use
    only if continuous tilt control is uncomfortable.

The exact choice should be tested with the actual board and sensor.
Avoid claiming that the device measures a physically drawn trajectory
unless the implemented mapping genuinely supports that interpretation.

## Phase 0: hardware and serial audit

Before implementing the game-side parser, identify: - Exact Arduino
board and accelerometer sensor model. - Current Arduino IDE/library
setup. - Which analog/digital/serial pins and libraries are already used
by the Sensor Kit. - Available physical buttons or other controls. -
Serial port and baud rate configuration. - Whether the current Arduino
sketch already reads sensor values. - Whether the game has a serial-port
utility or external dependency.

Do not assume a specific accelerometer chip or Arduino library. Test the
sensor independently before integrating it into the game.

## Data pipeline

``` text
Accelerometer
    |
Arduino sampling sketch
    |
Structured serial message
    |
Game-side serial reader
    |
Parser + validity checks
    |
Calibration + filtering + dead zone
    |
Tilt/gesture-to-cursor mapper
    |
2D cursor position
    |
World/editor coordinate mapper
    |
RawPathSample stream
    |
Existing PathProcessor
    |
Canonical Path
    |
Existing PathFollower
```

The serial reader should not directly edit OpenGL objects or car state.
It should produce sensor samples or mapped cursor updates that the input
controller consumes.

## Arduino serial protocol

Use a small, versioned, newline-delimited protocol that is easy to
inspect in a serial monitor. Example:

``` text
ACCEL,1,12345,0.02,-0.14,0.98
```

Possible fields: - Prefix `ACCEL`. - Protocol version `1`. - Arduino
timestamp in milliseconds. - X, Y, and Z acceleration readings in
documented units.

This is only an example format. Confirm the sensor's actual output units
and axes. If the board exposes normalized values rather than physical
units, document that fact.

Requirements: - One complete message per line. - Fixed field count and
separator. - Newline terminator. - No debug prose mixed into the data
stream. - Clear handling for malformed lines, missing fields,
out-of-range values, and serial disconnects. - A version field so the
protocol can evolve without silent parsing errors.

If control events are needed, either use a separate message type or keep
start/stop/clear controls on the PC keyboard initially. Prefer the
latter for the first prototype because it reduces hardware complexity.

## Arduino sampling

-   Sample at a consistent, documented rate supported by the sensor and
    board.
-   Use non-blocking timing where possible; avoid long `delay()` calls
    that starve sampling.
-   Send only the data needed for the mapping and debugging.
-   Avoid flooding the serial port beyond what the PC can read.
-   Keep sensor acquisition and serial formatting simple.
-   Test output in the serial monitor before connecting to the game.

Do not add a display, buzzer, or unrelated sensor functionality to this
sketch unless the existing assignment requires it.

## Game-side serial reader

Use the project's existing platform and dependency constraints. On
Windows, the project may need a small Win32 serial wrapper if no serial
library is already available. Do not add a cross-platform serial
dependency without approval.

Requirements: - Configure the COM port and baud rate outside the core
mapping logic. - Read complete lines incrementally; do not assume one OS
read equals one full message. - Avoid blocking the game/render loop
while waiting for serial data. - Use a bounded input buffer and recover
from malformed or overlong lines. - Expose connection status,
last-sample time, parsed values, and error count for diagnostics. -
Detect stale data; if no valid sample arrives for a configurable
timeout, stop recording or freeze the cursor safely and show a
warning. - Close the port cleanly when the mode exits or the application
shuts down.

If a worker thread is necessary, define ownership and shutdown behaviour
carefully. Do not let a background thread access OpenGL or mutate game
objects. A simple non-blocking polling implementation may be enough for
the first prototype.

## Calibration, filtering, and mapping

### 1. Calibration

Provide a short calibration step: - Ask the user to hold the
board/device in a neutral orientation. - Collect a small batch of
readings. - Calculate a baseline/neutral value. - Show a clear ready
state once calibration is complete. - Allow recalibration without
restarting the game.

### 2. Filtering and dead zone

-   Apply a modest low-pass filter or exponential moving average to
    reduce jitter.
-   Apply a dead zone around neutral so small movements do not move the
    cursor.
-   Avoid so much smoothing that the cursor feels delayed.
-   Make filter and dead-zone values configurable.

### 3. Map to cursor movement

For tilt-to-cursor velocity, a conceptual mapping is:

``` text
tilt = filteredReading - neutralBaseline
tiltOutsideDeadZone = applyDeadZone(tilt)
cursorVelocity = sensitivity * tiltOutsideDeadZone
cursorPosition += cursorVelocity * deltaTime
cursorPosition = clampToEditorBounds(cursorPosition)
```

This is pseudocode. The real mapping depends on the sensor axes, device
orientation, and whether the readings are raw acceleration, normalized
values, or already processed tilt angles.

Use `deltaTime` so movement is not dependent on frame rate. Define axis
signs with a calibration test: tilt left/right and forward/backward, and
verify cursor direction. Provide a sensitivity setting and a clear
reset-to-center action.

### 4. Sampling path points

While recording: - Convert cursor coordinates to the same world-space
coordinates used by the mouse editor. - Record a sample only after a
minimum time interval or movement threshold, so the path is not flooded
with duplicates. - Timestamp samples. - Preserve raw sensor values in
optional diagnostic logs. - End the stroke explicitly when recording
stops.

The same `PathProcessor` should then validate, resample, smooth, and
calculate cumulative distances. Do not create a second sensor-specific
path format.

## User interaction

Suggested flow: 1. Select Arduino mode. 2. Connect to the configured
serial port. 3. Calibrate neutral orientation. 4. Verify the virtual
cursor responds to tilt. 5. Press Start Recording. 6. Move the cursor
using tilt to create a path. 7. Press Stop Recording. 8. Preview the
path and optionally clear/redraw. 9. Confirm the path. 10. Let the
existing follower drive it. 11. Stop, edit, or switch to WASD when
desired.

For the first prototype, PC keyboard shortcuts for start/stop/clear are
acceptable. The interface should show clear instructions so the user
does not need to know the protocol or sensor internals.

## File organization

Adapt to the repository and platform:

``` text
ArduinoPathInput.h / ArduinoPathInput.cpp
AccelerometerMapper.h / AccelerometerMapper.cpp
SerialPort.h / SerialPort.cpp             # only if no existing utility exists
```

Keep the Arduino sketch in a clearly named separate directory, for
example:

``` text
arduino/
  AccelerometerPathInput/
    AccelerometerPathInput.ino
```

Do not place Arduino headers in the core `Path` or `PathFollower`
modules. Avoid including Windows serial headers in shared
path-processing code.

## Error handling

Handle: - Port unavailable or already in use. - Board unplugged during
recording. - Invalid/malformed serial data. - Stale sensor values. -
Calibration not completed. - Sensor values outside expected bounds. -
Cursor reaching editor bounds. - User stops recording with too few
samples. - Switching to mouse or WASD mode. - Game shutdown while serial
reading is active.

Fail safely: a serial error must not crash the game, teleport the car,
or leave the game permanently in Arduino mode. Show a useful status
message and allow the user to return to mouse or WASD.

## Testing

### Hardware-independent tests

-   Parse valid protocol lines.
-   Reject malformed and version-mismatched messages.
-   Handle partial messages spanning multiple reads.
-   Handle multiple lines in one read.
-   Handle stale timestamps and invalid values.
-   Calibration baseline and dead-zone behaviour.
-   Filtering response and cursor movement using recorded/synthetic
    samples.
-   Cursor clamping and axis inversion.
-   Path sampling thresholds.

### Hardware tests

1.  Confirm sensor axes and units with the board stationary.
2.  Tilt one axis at a time.
3.  Verify neutral calibration.
4.  Check cursor jitter when stationary.
5.  Check cursor speed at small and large tilt.
6.  Check cursor behaviour after returning to neutral.
7.  Record a straight path, an S-curve, and a loop-like path.
8.  Unplug the board during recording.
9.  Reconnect and recalibrate.
10. Confirm the same recorded path can be replayed through the follower.

### End-to-end tests

-   Arduino-generated path and mouse-generated path use the same `Path`
    format.
-   The follower can follow either without sensor-specific code.
-   WASD remains functional after serial disconnect.
-   Stale data never causes uncontrolled cursor movement.
-   The path can be stopped, previewed, cleared, and redrawn.

## Experiment design and logging

Compare at least: - WASD baseline. - Mouse-drawn path. - Arduino-created
path.

Where possible, use the same canonical path to measure follower
performance independently of the input method. For the input comparison,
record both the path creation process and driving outcome.

Suggested CSV columns: - `session_code` - `trial_index` - `input_mode` -
`track_reference_visible` - `path_creation_seconds` -
`raw_sample_count` - `processed_point_count` - `path_length` -
`lap_completed` - `lap_time_seconds` - `mean_path_deviation` -
`max_path_deviation` - `collision_count` - `track_departure_count` -
`redraw_count` - `user_intuitiveness_rating` - `user_comments`
(optional, avoid identifying details)

Define path deviation consistently. One useful measure is the distance
from the car's ground-plane position to the nearest valid segment of the
intended path. For closed loops, handle wraparound and
self-intersections carefully. Record whether measurements are computed
during the whole run or only while the car is moving.

For usability, use the same instructions and time limits across
participants. Counterbalance input-method order where feasible to reduce
practice/order effects. Separate results about the input interface from
results about the follower itself.

## Milestones and acceptance criteria

### Milestone 1: sensor proof of concept

-   Arduino emits stable, structured accelerometer readings.
-   Units and axes are documented.

### Milestone 2: serial connection

-   Game reads and parses messages without blocking rendering.
-   Disconnects and malformed input are handled safely.

### Milestone 3: cursor mapping

-   Neutral calibration, filtering, dead zone, sensitivity, and cursor
    bounds work.
-   The user can reliably draw a simple path.

### Milestone 4: path integration

-   Arduino samples become raw path samples and then a canonical `Path`.
-   The same preview, validation, clear/redraw, confirm, and follow
    workflow works.

### Milestone 5: evaluation

-   CSV logging captures comparable results for WASD, mouse, and
    Arduino.
-   The experiment procedure and known sensor limitations are
    documented.

Acceptance criterion: a user can calibrate the accelerometer, create a
path using the documented mapping, preview and confirm it, and have the
existing path follower follow it without changing the path follower or
car physics.

## Coding-agent prompt

> Implement Stage 3 only, using the existing canonical path and
> mouse-editor workflow. First inspect the actual Arduino Sensor Kit
> hardware, current sketch, sensor libraries, OS, build setup, and
> existing serial utilities. Do not assume a specific accelerometer
> model. Implement a structured newline-delimited serial protocol and a
> non-blocking game-side reader, with robust parsing, connection status,
> stale-data handling, and clean shutdown. Implement a calibrated
> tilt/gesture-to-cursor mapping with filtering, dead zone, sensitivity,
> bounds, and explicit start/stop recording. Convert the mapped cursor
> into the same raw world-space samples and canonical Path used by the
> mouse editor. Do not double-integrate raw acceleration as the default
> position estimator. Keep hardware code separate from Path and
> PathFollower, avoid unnecessary dependencies, preserve WASD, and test
> parsing/mapping independently from hardware where possible. Document
> protocol fields, sensor units/axes, calibration, changed files, build
> steps, and known limitations.

## Repository implementation notes

### Sensor Kit firmware

The sketch at `MyAVTM_Template/arduino/AccelerometerPathInput/AccelerometerPathInput.ino`
targets the Grove LIS3DHTR 3-axis accelerometer included with the Seeed
Studio Arduino Sensor Kit. It uses the LIS3DHTR I2C address `0x19`, selects
the ±2 g range, and requests 50 Hz output. Install the Seeed Studio
`LIS3DHTR` Arduino library, select the Sensor Kit's Arduino-compatible
board, and upload this sketch. The sketch emits one CSV record every 20 ms
and sends no diagnostic text to the data stream.

The protocol is `ACCEL,1,<millis>,<x>,<y>,<z>\n`. The acceleration values
are the library's acceleration readings in g. X and Y are used as the
cursor's horizontal and vertical tilt controls; Z is retained for
diagnostics/calibration and is not integrated. The board's physical axis
orientation and signs must be checked by tilting the actual kit. The game
provides `invertX` and `invertY` in `AccelerometerMapperConfig` to correct
direction without changing path code. This mapping estimates tilt-relative
velocity; it does not integrate acceleration into position.

### Game connection and controls

Set `AVTM_ACCEL_COM` to the Arduino's COM port and optionally
`AVTM_ACCEL_BAUD` to the sketch baud rate (default `COM3` and `115200`).
Close Arduino Serial Monitor before connecting because only one application
can own the port. In the path editor:

| Key | Action |
| --- | --- |
| `I` | Connect/disconnect the configured COM port |
| `G` | Recalibrate neutral orientation (hold the board still) |
| `B` | Start a new Arduino path stroke after calibration |
| `N` | Stop recording and preview the stroke |
| `O` | Return the virtual cursor to the editor center |
| `C` | Clear the path; mouse drawing remains available |
| `Enter` / `Space` | Confirm a valid path and use the existing follower |
| `E` | Exit the editor and close the serial port |

Connection starts a 30-sample neutral calibration automatically. The mapper
applies an exponential low-pass filter, a 0.04 g dead zone, a configurable
tilt response/sensitivity and speed limit, delta-time motion, and bounds from
the editor's orthographic view. Recording samples the cursor into the same
`PathEditor` raw samples and `PathProcessor` canonical `Path` used by the
mouse editor. A malformed line increments the diagnostic error count; a
disconnect or one second without a valid sample stops an active stroke.
Path following and car physics do not consume sensor data.

### Changed files and validation

- `AccelerometerInput.h/.cpp`: protocol validation, calibration, filtering,
  dead zone, cursor mapping, bounds, and axis inversion.
- `SerialPort.h/.cpp`: non-blocking Windows COM reader and diagnostics.
- `GameManager.*`, `InputManager.cpp`, `PathEditor.h`, and
  `PathVisualizer.*`: editor controls, shared path recording, status, and
  cursor marker.
- `AccelerometerInputTests.*`: hardware-independent parser, mapper, and
  shared-path tests; run them with the application's existing `--test`
  option after building.
- `arduino/AccelerometerPathInput/AccelerometerPathInput.ino`: kit sketch.

Build the C++ application with the existing `MyAVTM_Template` Visual Studio
project (Debug/x64 or Release/x64). Run the built executable with `--test`
to execute the path follower, path processor/editor, and accelerometer input
tests. In PowerShell, set `$env:AVTM_ACCEL_COM = "COM5"` (replace `COM5` with
the port shown by Windows Device Manager); set `$env:AVTM_ACCEL_BAUD` to
`115200` if using a non-default sketch baud rate, then start the app.

The parser and mapper tests do not require the board. Hardware validation
still needs to confirm the installed LIS3DHTR library version and I2C address,
neutral jitter, physical X/Y signs, cursor comfort, and unplug/reconnect
behavior on the actual kit. The sketch intentionally has no device-specific
fallback if the accelerometer does not initialize; in that case the game
will report that valid samples are not arriving. COM defaults may need
adjustment per machine. Experiment CSV outcome logging described above is
not part of this input-path implementation.
