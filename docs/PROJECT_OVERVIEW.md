# Project Overview --- User-Drawn Path Following for a Physics-Based Car Game

## 1. Project goal

Extend the existing C++/OpenGL racing game so a user can define a
driving trajectory using a mouse or an Arduino Sensor Kit accelerometer,
and then have the car follow that trajectory using the game's existing
physics.

The project has two goals:

1.  **Engineering goal:** create a reusable pipeline from input capture
    to a world-space path to physics-aware path following.
2.  **Research goal:** evaluate the usability of the input interface and
    how accurately the resulting path can be followed.

The game currently uses WASD controls. Preserve that control mode as a
baseline and fallback.

## 2. Scope and guiding principles

-   Preserve the existing C++ and OpenGL architecture, physics, scene
    graph, rendering, collision, and lap-scoring systems wherever
    possible.
-   Do not introduce a new rendering framework or a math dependency such
    as GLM.
-   Reuse the project's own `Vector3` or existing vector utilities if
    present. Otherwise, use the existing `float[3]` conventions or
    introduce a small, compatible path-specific vector helper only after
    inspecting the code.
-   Keep input capture, path representation, path editing, path
    following, and experiment logging separate.
-   All path sources must produce the same canonical path format. The
    follower must not know whether a path came from a mouse, Arduino, or
    test data.
-   Build and test in small steps. Do not implement all stages in one
    large change.
-   The player remains responsible for obstacle avoidance. The follower
    tracks the intended path; it is not an obstacle-avoidance AI.
-   Do not silently replace the current movement model. The path
    follower should request steering/throttle/brake-like controls, while
    the existing physics determines the resulting movement.

## 3. User experience

The intended workflow is:

1.  Enter **Path Edit** mode.
2.  Choose the track-visible view or a blank view.
3.  Draw a path with the mouse, or choose Arduino recording mode.
4.  Preview the raw samples and the processed/smoothed path.
5.  Clear, redraw, or replace the path as needed.
6.  Confirm the path and enter **Follow Path** mode.
7.  Optionally switch between mouse, Arduino, and WASD control modes.
8.  Record performance and usability observations for evaluation.

A hybrid interaction is planned: users can either finish and confirm a
path before driving or enable an optional live-drawing mode. The initial
implementation should prioritize **record → preview/edit → confirm →
follow**. Live drawing is a later extension because it introduces
path-end, update, and controller-transition edge cases.

## 4. Architecture

``` text
Mouse Editor -----------\
                         \
Arduino Input -> Calibration -> Filtering/Mapping -> Raw Path Samples
                                                   |
                                                   v
                                           Path Processing
                                     (validation, resampling,
                                      optional smoothing)
                                                   |
                                                   v
                                           Canonical Path
                                                   |
                                                   v
                                      Path-Following Controller
                                                   |
                                      steering/throttle/brake
                                                   |
                                                   v
                                       Existing Car Physics
                                                   |
                                                   v
                                    Existing Game / Lap Scoring

WASD --------------------------------> Existing Car Controls

Experiment Logger <--- input mode, timestamps, path metrics,
                       lap results, completion time, user ratings
```

### Recommended modules

-   `Path`: canonical path data, independent of input source and
    rendering.
-   `PathProcessor`: validates, resamples, and optionally smooths raw
    samples.
-   `PathEditor`: mouse drawing, preview, clear, and redraw
    interactions.
-   `PathFollower`: computes desired control inputs from the canonical
    path.
-   `ArduinoPathInput`: reads and parses sensor messages from the
    Arduino connection.
-   `AccelerometerMapper`: calibrates and maps sensor gestures into 2D
    path samples.
-   `InputModeController` or equivalent: selects WASD, mouse-drawn path,
    or Arduino-drawn path.
-   `ExperimentLogger`: optional CSV logging of conditions and outcomes.
-   Existing `Car`, physics, scene graph, collision system, and lap
    manager remain the owners of their current responsibilities.

These are conceptual module names, not a mandate to create every class
immediately. First inspect the repository and adapt names and locations
to its conventions.

## 5. Shared data model and interfaces

Use the project's existing vector type if possible. The following is
illustrative pseudocode; adapt it to actual project conventions.

``` cpp
struct PathPoint
{
    Vector3 position;     // World-space point on the driving plane
    float cumulativeDistance;
};

struct Path
{
    std::vector<PathPoint> points;
    bool closedLoop;
    bool valid;
};

struct RawPathSample
{
    Vector3 position;
    double timestampSeconds;
};

struct PathFollowOutput
{
    float steering;       // Normalized, e.g. [-1, 1]
    float throttle;       // Normalized, e.g. [0, 1]
    float brake;          // Normalized, e.g. [0, 1]
    bool finished;
};
```

If the current code has no `Vector3`, do not add a broad math library
just for this feature. Reuse the project's custom `Vector3` if it is
already being developed; otherwise, add only the minimal operations
needed and avoid incompatible duplicate types.

Recommended conceptual APIs:

``` cpp
class PathProcessor
{
public:
    bool BuildPath(const std::vector<RawPathSample>& raw,
                   bool closedLoop,
                   Path& output);
};

class PathFollower
{
public:
    void SetPath(const Path& path);
    void Reset();
    PathFollowOutput Update(const CarState& car, float deltaTime);
};
```

`CarState` is an adapter/view of the state the car already exposes:
position, velocity, facing direction, current speed, and any steering
limits. Do not duplicate ownership of car physics state.

The exact signatures must be adjusted after inspecting the current
codebase. Avoid circular includes: path data should not include `Car`,
OpenGL, scene graph, or Arduino headers.

## 6. Coordinate systems

Define coordinate mapping before implementing drawing:

-   **Editor coordinates:** mouse or sensor coordinates, usually 2D.
-   **Track/world coordinates:** the coordinate system in which the car
    and track exist.
-   **Render coordinates:** screen pixels and camera projection.

The editor must convert screen coordinates to world coordinates. If the
track lies on a fixed plane, intersect a mouse ray with that plane or
use the existing camera/unprojection helpers. Do not guess a fixed
pixels-to-world scale unless the game uses a fixed top-down orthographic
view.

For a separate editor window, use an explicit mapping between
editor-space bounds and world-space track bounds. Keep that mapping in
one component and test it at known corners and the track center.

The path's vertical coordinate should match the game's driving plane.
Avoid accidentally drawing paths at a different height from the car or
collision plane.

## 7. Path-following strategy

Start with a robust, explainable controller:

1.  Find the closest valid segment near the previous path progress, not
    just the globally closest point.
2.  Project the car position onto that segment to estimate progress
    along the path.
3.  Select a look-ahead target ahead of that progress.
4.  Compute the target direction and steering error relative to the
    car's facing direction.
5.  Produce bounded steering, throttle, and braking requests.
6.  Let the existing physics update velocity and position.
7.  Reconcile the controller's progress with the car's actual movement.

A look-ahead distance that grows with speed is a useful starting point.
Use a configurable minimum and maximum so it does not become unstable at
low speed or skip corners at high speed.

For physics integration, the follower should produce control requests
rather than directly teleporting the car or overwriting its position.
The existing acceleration, drag, speed limits, turning behaviour, and
collision response should continue to operate. Add an adapter to the
car's current control interface rather than duplicating the movement
equations.

Important edge cases: - Closed-loop wraparound. - Path endpoints for
open paths. - Self-intersections and nearby parallel segments. -
Low-speed starting and stopping. - Reversing, if the car supports
reverse. - Large `deltaTime` values or dropped frames. - Invalid or
too-short paths. - A path that is physically impossible to follow at the
current speed.

## 8. Mouse editor recommendation

Start in the **existing game window** if the project already supports
reliable mouse input and an editor overlay can be added without
disrupting rendering. This avoids inter-process communication and
duplicate window/context setup.

Keep the editor logic independent from the window, though, so it can
later be hosted in a separate window. A separate editor is justified if
the current window/input/rendering architecture makes overlays
significantly harder or if independent editor testing is important.

Support two visual modes: - **Track-visible:** show a top-down track
reference and path overlay. - **Blank:** hide the track reference while
preserving the same coordinate mapping and canvas bounds.

A toggle should change only the reference visibility, not the path's
coordinate mapping. Provide clear feedback for edit, preview, and follow
modes.

## 9. Arduino accelerometer strategy

An accelerometer measures acceleration, not absolute position.
Double-integrating raw accelerometer data to recover a drawn 2D
trajectory will drift rapidly and is not a reliable baseline.

For a practical prototype, investigate a **gesture/tilt-to-cursor**
mapping: - Calibrate a neutral orientation. - Map tilt/acceleration axes
to cursor velocity or path movement. - Apply a dead zone and modest
filtering. - Convert movement into world-space path samples. - Include a
button, keyboard key, or serial command for start/stop/clear if the
available kit does not have a convenient physical control.

If the exact Sensor Kit board exposes an accelerometer module, verify
its model, axis directions, sample rate, and serial output before
selecting a library or protocol. Do not assume a specific sensor or
library without inspecting the hardware and current Arduino sketch.

Keep raw sensor values and mapped cursor/path values available for
debugging. If an absolute, physically drawn path is required, assess the
limitations experimentally and explain them in the report.

## 10. Evaluation plan

Measure usability and performance separately.

### Performance metrics

-   Path-following deviation: mean and maximum lateral distance from the
    intended path.
-   Lap completion rate.
-   Lap completion time.
-   Number of collisions or track departures.
-   Path creation time.
-   Path length and smoothness, where useful.
-   Controller recovery behaviour after a collision or large deviation.

### Usability measures

-   Time to understand the interface.
-   Time to create a usable path.
-   Number of redraws or corrections.
-   User-rated intuitiveness and workload using a short, consistent
    questionnaire.
-   User comments about predictability and control.

### Experimental controls

-   Keep track, car physics, target path, and follower settings fixed
    when comparing input methods.
-   Preserve WASD as a baseline where feasible.
-   Record the input method, path mode, user/session identifier using
    non-identifying codes, trial number, configuration, and outcomes.
-   Separate the effect of path drawing from the effect of path
    following: the same canonical path can be replayed to test the
    follower independently.
-   Obtain any required instructor/ethics approval before collecting
    participant data.

## 11. Suggested repository layout

Adapt to the existing repository rather than moving working files
unnecessarily.

``` text
src/
  path/
    Path.h
    Path.cpp
    PathProcessor.h
    PathProcessor.cpp
    PathFollower.h
    PathFollower.cpp
  input/
    PathEditor.h
    PathEditor.cpp
    ArduinoPathInput.h
    ArduinoPathInput.cpp
    AccelerometerMapper.h
    AccelerometerMapper.cpp
  experiment/
    ExperimentLogger.h
    ExperimentLogger.cpp
tests/                         # Or the project's existing test location
docs/
  PROJECT_OVERVIEW.md
  01_PATHFOLLOWING_AI.md
  02_PATH_EDITOR.md
  03_EXTERNAL_INPUT.md
```

Do not create empty wrappers for every proposed module if the current
project is small. Keep the implementation minimal and aligned with the
existing build system.

## 12. Milestones

1.  **Repository audit:** identify car update order, movement controls,
    coordinate system, input handling, camera projection, build
    settings, and lap logic.
2.  **Static-path prototype:** follow a hard-coded path with debug
    visualization.
3.  **Controller validation:** closed loop, corners, start/stop,
    collisions, and different speeds.
4.  **Mouse path editor:** draw, preview, clear, redraw, and confirm.
5.  **Path processing:** raw sample preservation, validation,
    resampling, and optional smoothing.
6.  **Arduino prototype:** establish serial communication and verify
    sensor axes and calibration.
7.  **Input integration:** produce canonical path data from Arduino
    gestures.
8.  **Experiment logging:** capture path and trial metrics.
9.  **Evaluation and tuning:** run repeatable tests, document
    limitations, and compare input methods.

## 13. Definition of done

The project is ready for evaluation when: - The original WASD mode still
works. - The AI follows a predefined path through the existing
physics. - The user can draw, preview, clear, and replace a path with
the mouse. - The track-visible and blank editor modes share a consistent
mapping. - Arduino input can create a path through a documented
calibration/mapping method. - Input sources feed the same canonical path
representation. - The user can stop, edit, and restart without
restarting the application. - Core performance metrics can be recorded
repeatably. - Build instructions, limitations, and the test procedure
are documented.

## 14. Global instructions for the coding agent

Before editing: 1. Inspect the repository, current build configuration,
`Car` update loop, control input path, scene graph, camera, track
coordinate system, collision handling, and lap scoring. 2. Report the
relevant existing classes/functions and propose the smallest integration
points. 3. Do not assume APIs or class names from this document exist
already. 4. Do not use GLM or introduce a new third-party dependency
unless explicitly approved. 5. Preserve existing gameplay and test WASD
after each integration change. 6. Make changes in small compilable
increments; state the files changed and the build/test performed. 7. If
a necessary project detail is unclear, ask before making a large
architectural change.
