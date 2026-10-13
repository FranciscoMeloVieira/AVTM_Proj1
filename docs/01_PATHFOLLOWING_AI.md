# Stage 1 --- Physics-Aware Path-Following AI

## Objective

Implement an autonomous path follower that can follow a predefined test
path using the existing car physics. Do not implement mouse drawing or
Arduino input in this stage. The follower should accept a canonical path
and output control requests, while the existing car update logic
continues to determine actual motion.

## Non-goals

-   No obstacle avoidance.
-   No direct teleportation or position snapping to the path.
-   No replacement of the existing physics model.
-   No mouse, camera, or Arduino input in this stage.
-   No changes to lap scoring unless a small integration adjustment is
    necessary and justified.

## Phase 0: repository audit

Before coding, locate and document: - The `Car` class and its position,
velocity, acceleration, facing/rotation, steering, throttle, brake, and
speed-limit members. - The exact function that updates car movement and
the order of input, physics, collision, and rendering updates. - Whether
turning is based on Euler rotation, a facing vector, or another
representation. - Whether velocity can point opposite to facing
direction and how reverse works. - The world's driving plane and the
coordinate axes used by the track. - The collision correction and
checkpoint/lap update sequence. - The current input handling and how
WASD is translated into movement. - Existing custom `Vector3`
implementation, if any.

Do not guess. If movement controls are currently embedded in a
monolithic function, propose a small adapter or control-command
structure without refactoring unrelated code.

## Proposed data types

Use existing project types where possible. These definitions are
conceptual and should be adapted to the actual code.

``` cpp
struct PathPoint
{
    Vector3 position;
    float cumulativeDistance;
};

struct Path
{
    std::vector<PathPoint> points;
    bool closedLoop;
    bool valid;
};

struct CarState
{
    Vector3 position;
    Vector3 velocity;
    Vector3 forward;
    float speed;
};

struct PathFollowOutput
{
    float steering; // normalized [-1, 1]
    float throttle; // normalized [0, 1]
    float brake;    // normalized [0, 1]
    bool finished;
};
```

`CarState` should be populated from the real car state each update; it
must not become a second source of truth. If the current car uses
arrays, either adapt at the boundary or keep the data layout consistent
with the codebase.

## Controller algorithm

### 1. Track progress on the path

A simple global nearest-waypoint search can jump to the wrong branch at
intersections or near parallel track sections. Use segment projection
and maintain progress.

For each candidate segment A→B: 1. Compute `AB = B - A`. 2. Compute
`t = dot(carPosition - A, AB) / dot(AB, AB)`. 3. Clamp `t` to `[0, 1]`.
4. Compute the closest point `Q = A + t * AB`. 5. Compute distance from
car position to `Q`. 6. Convert `t` to a distance-along-path value using
the segment's cumulative distance.

Search a local forward window around the previous progress first. If no
plausible candidate is found, allow a wider recovery search. For a
closed loop, handle wraparound. Ignore zero-length segments.

Do not blindly restrict the search to only forward segments: after a
collision the car may be displaced or pushed backwards. Define a
recoverable progress window and a controlled fallback.

### 2. Choose a look-ahead target

Let: - `s` be estimated progress along the path, - `L` be look-ahead
distance, - `sTarget = s + L`.

Use a configurable speed-aware rule such as:

``` text
L = clamp(minLookAhead + lookAheadGain * abs(speed),
          minLookAhead,
          maxLookAhead)
```

These are tunable parameters, not universal constants. Start with
conservative values and expose them in one configuration structure. For
a closed loop, wrap `sTarget` around total path length. For an open
path, clamp it to the endpoint.

Interpolate the target position along the segment containing `sTarget`;
do not only select a distant sample if the path spacing is irregular.

### 3. Calculate steering request

Compute the horizontal direction from the car to the target, then
compare it to the car's current forward direction. Use the game's
driving plane, not all three axes if vertical movement is irrelevant.

Conceptually:

``` text
targetDirection = normalize(targetPosition - carPosition)
steeringError = signedAngle(carForward, targetDirection, drivingPlaneNormal)
steering = clamp(steeringError / maxSteeringAngle, -1, 1)
```

The signed-angle convention must match the existing car's steering sign.
Verify with a small test: target left should turn left, target right
should turn right.

If the car uses a facing vector, use it directly. If it uses rotation
angles, derive forward from the current convention. Do not introduce a
second independent heading representation.

### 4. Request a reasonable speed

The first version can use a fixed target speed. Later, use
curvature-aware speed control: - Estimate path curvature using nearby
path directions or the change in tangent. - Reduce target speed for
sharp turns. - Use throttle when below target speed and brake or coast
when above it. - Respect the existing maximum speed, reverse rules, and
acceleration limits.

Do not add complex PID tuning before the basic follower works. Start
with bounded proportional steering and a simple speed target, then tune
from repeatable tests.

### 5. Send control requests to existing physics

The follower should return steering/throttle/brake values. The car or a
thin adapter applies them through the same movement pathway as human
input.

Recommended update sequence, subject to the real game loop: 1. Select
active input mode. 2. If path-follow mode is active, ask the follower
for control values. 3. Feed those values into the car's existing
control/physics update. 4. Run existing collision detection and
response. 5. Update checkpoint/lap logic. 6. Render the scene and
optional path debug overlay.

Do not apply both WASD and AI controls in the same frame unless an
explicit override policy exists. Add a clear mode switch and ensure
switching back to WASD resets controller state appropriately.

## Path validity and endpoint behaviour

Reject or disable paths that: - Have fewer than two distinct points. -
Contain non-finite coordinates. - Have near-zero total length. - Contain
too many duplicate consecutive samples without meaningful movement.

For open paths, define endpoint behaviour explicitly: - Stop: reduce
target speed as the car approaches the endpoint and set `finished`. -
Loop: only if explicitly configured as a closed loop. - Do not silently
wrap an open path.

For closed loops, ensure the final segment connects the last point to
the first and cumulative distance includes that segment.

## Debug visualization

Provide optional debug rendering for: - Raw or processed path
polyline. - Current estimated progress point. - Look-ahead target. -
Current car forward vector. - Steering direction/error, if useful. -
Path index/progress and target speed as text or console diagnostics.

Use the project's current rendering/debug facilities. Do not create a
new rendering subsystem just for this.

## Suggested files

Adapt names and directories to the repository:

``` text
Path.h / Path.cpp
PathFollower.h / PathFollower.cpp
PathFollowerConfig.h (optional; may be a struct in PathFollower.h)
```

`PathProcessor` may be added in Stage 2 if path smoothing/resampling is
not needed for the first static path. Keep the initial path creation
helper simple.

## Configuration parameters

Centralize and document parameters: - `minLookAhead` - `maxLookAhead` -
`lookAheadGain` - `targetSpeed` - `turnSpeedReduction` or curvature
speed settings - `progressSearchWindow` - `recoveryDistance` -
`steeringGain` - `arrivalDistance` / endpoint slow-down distance

Use world units consistent with the track. Do not scatter magic numbers
through update code.

## Test plan

### Unit/logic tests where the project supports them

-   Segment projection for a point on, beside, and beyond a segment.
-   Correct cumulative distance and target interpolation.
-   Closed-loop wraparound.
-   Open-path endpoint handling.
-   Zero-length segment skipping.
-   Steering sign for targets to the left and right.
-   Invalid path rejection.
-   Recovery after progress becomes stale.

### In-game tests

1.  Straight line at low speed.
2.  Straight line at moderate speed.
3.  Wide 90-degree turn.
4.  Tight turn with reduced target speed.
5.  Closed oval loop and repeated laps.
6.  Path near itself or crossing itself.
7.  Start from rest.
8.  Stop near an open endpoint.
9.  Collision that pushes the car off the path.
10. Switch AI → WASD → AI.
11. Confirm collisions, checkpoints, and lap scoring still work.
12. Confirm the original WASD mode is unchanged.

Record observed failures and tune one parameter at a time. Avoid using
lap completion alone as the only success metric.

## Milestones and acceptance criteria

### Milestone 1: static path

-   A hard-coded path is represented in world coordinates.
-   It can be drawn using existing debug rendering.
-   The project builds.

### Milestone 2: controller

-   The car follows a straight path and a wide curve.
-   Steering sign and speed behaviour are correct.
-   Existing physics still moves the car.

### Milestone 3: closed loop and recovery

-   The car can repeatedly follow a closed test loop.
-   Progress does not jump unexpectedly at the loop boundary or nearby
    path segments.
-   Collision response remains owned by the existing game.

### Milestone 4: mode integration

-   WASD and AI are mutually exclusive active input modes.
-   Switching modes is reliable.
-   Existing lap/checkpoint logic continues to function.

Acceptance criterion: the AI can follow a representative predefined path
repeatedly without teleporting the car, while the current physics,
collision, and scoring systems remain in use.

## Coding-agent prompt

> Implement Stage 1 only. First inspect the repository and report the
> actual car movement, input, coordinate, and update-loop APIs. Then
> implement a minimal path data structure and physics-aware path
> follower that follows a hard-coded test path. Reuse the existing
> vector type and physics. The follower must output
> steering/throttle/brake requests through the existing movement
> pathway; it must not teleport the car or implement obstacle avoidance.
> Support segment projection, persistent progress, speed-aware
> look-ahead, open/closed paths, endpoint handling, and optional debug
> visualization. Keep WASD working and make AI/WASD modes mutually
> exclusive. Do not add third-party dependencies or refactor unrelated
> systems. Build and test incrementally, list changed files, document
> parameters, and report any assumptions or unresolved issues.
