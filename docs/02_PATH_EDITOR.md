# Stage 2 --- Mouse-Based Path Editor

## Objective

Let a user create and revise a path with the mouse, preview the path,
and hand the confirmed path to the Stage 1 follower. Support a
track-visible editor mode and a blank/reference-hidden mode. Preserve
raw input samples while producing a processed path for driving.

The editor should be developed before Arduino integration because it
validates the canonical path data model, coordinate mapping, path
processing, and user workflow without hardware complexity.

## Core workflow

1.  Enter Path Edit mode; the car should not drive under AI control
    while drawing.
2.  Select track-visible or blank view.
3.  Press and hold the configured mouse button to record samples.
4.  Release to finish the stroke.
5.  Preview raw samples and processed path.
6.  Clear, redraw, or confirm.
7.  On confirm, validate the path and switch to Follow Path mode.
8.  Stop following at any time and return to edit mode without
    restarting the application.

Initial priority is record → preview/edit → confirm → follow. Add live
drawing only after the stable workflow works.

## Editor location decision

Prefer the existing game window if it can support mouse input and a 2D
overlay without a large rendering/input refactor. This avoids a second
window, context sharing, and path-transfer protocol.

Keep the editor's core logic independent from the window. If the
existing renderer or input system makes an in-game overlay
disproportionately difficult, use a separate editor window with an
explicit world-to-editor mapping. Do not build two full editor
implementations.

### In-game editor

Advantages: - Direct view of the actual track. - No separate window or
communication protocol. - Easy path-to-world mapping if a top-down view
exists.

Risks: - UI overlay and camera input can conflict. - Mouse coordinates
may need ray-plane projection. - Existing scene rendering may not have a
UI abstraction.

### Separate editor

Advantages: - Simple 2D drawing surface. - Easier to provide blank and
track-reference modes. - Editor can be tested independently.

Risks: - Requires a second window/context or separate process. - Needs a
reliable mapping from editor coordinates to world coordinates. - May
require image export or a protocol to communicate with the game.

Decision gate: audit the current windowing/input/camera architecture
first. Choose the smallest implementation that can correctly map drawn
points into world space. Keep the Path Editor API independent of that
decision.

## Data model

Maintain two forms:

-   `RawPathSample`: the original recorded sample position and
    timestamp.
-   `Path`: validated, processed points used by the follower.

Suggested responsibilities:

``` cpp
class PathEditor
{
public:
    void BeginStroke();
    void AddSample(const Vector3& worldPosition, double timestampSeconds);
    void EndStroke();
    void Clear();
    bool HasSamples() const;
    const std::vector<RawPathSample>& GetRawSamples() const;
};
```

These signatures are conceptual. Adapt them to project conventions. The
editor should not own or update car physics.

## Coordinate mapping

This is a critical part of the implementation.

### In-game view

-   Convert mouse pixel coordinates into a camera ray using existing
    projection/view matrices or existing unprojection utilities.
-   Intersect that ray with the fixed driving plane.
-   Reject samples if the ray is parallel to the plane or the
    intersection is invalid.
-   Clamp or reject points outside the permitted track/editor bounds,
    according to a documented policy.
-   Set the vertical coordinate to the driving plane height.

If the game uses a top-down orthographic camera, a simpler inverse
mapping may be available. Use it only if it matches the actual camera
configuration.

### Separate editor

Define a mapping from editor rectangle `[0, width] × [0, height]` to
known world-space bounds. Make the mapping explicit and reversible. For
example, normalized editor coordinates `(u, v)` can map linearly into
world-space track bounds, with the vertical coordinate fixed to the
driving plane.

Test: - All four corners. - The center. - A known track landmark. -
Aspect-ratio changes and window resizing. - Whether screen Y must be
inverted. - Track-visible and blank modes using identical coordinates.

The track-visible toggle must only change the background/reference
rendering, not alter the path transform.

## Input and UI states

Use an explicit state machine rather than many loosely related booleans:

``` text
Driving
  -> Editing
Editing
  -> Recording
Recording
  -> Editing (stroke ended)
Editing
  -> Preview
Preview
  -> Editing (clear/redraw)
Preview
  -> Following (confirm valid path)
Following
  -> Editing (stop/edit)
```

Exact states can be simplified to fit the current game, but each action
must have a predictable effect.

Suggested controls (adapt to current bindings): - Enter/toolbar action:
enter or leave edit mode. - Mouse button: start/continue/end stroke. -
Clear action: discard raw and processed path. - Preview action: show the
path without driving. - Confirm action: validate and activate the
path. - Stop action: stop AI following and return to edit mode. - Track
visibility toggle: show/hide track reference. - Optional
undo-last-stroke if the drawing model supports multiple strokes.

Provide on-screen labels or clear console instructions. Avoid relying
only on undocumented keyboard shortcuts.

## Path processing

Implement a deterministic processing pipeline:

1.  **Validate raw samples**
    -   Require enough distinct samples.
    -   Reject non-finite values.
    -   Remove consecutive samples closer than a configurable minimum
        distance.
    -   Preserve the raw list for debugging and experiment analysis.
2.  **Optional resampling**
    -   Resample by approximately uniform world-space spacing so path
        behaviour does not depend heavily on mouse event frequency.
    -   Avoid creating excessive points.
3.  **Optional smoothing**
    -   Use a simple, dependency-free method such as a moving average or
        Catmull--Rom spline, only after confirming endpoint and
        overshoot behaviour.
    -   Preserve important corners or provide a smoothing strength
        parameter.
    -   Do not smooth across disconnected strokes unless the design
        explicitly joins them.
    -   Keep the raw path visible in debug mode so users can compare it
        with the processed path.
4.  **Calculate cumulative distance**
    -   Populate each processed point's distance along the path.
    -   For closed loops, include the last-to-first segment.
    -   Mark a path closed only when the user explicitly closes it or
        the UI provides a deliberate close-loop action. Do not close
        arbitrary open strokes automatically.
5.  **Validate final result**
    -   Reject a path that is too short, degenerate, outside allowed
        bounds, or contains invalid coordinates.
    -   Show a useful message and remain in edit mode if validation
        fails.

Use conservative defaults first. Over-smoothing can create a route the
user did not intend; no smoothing can create a route the car cannot
physically track. Compare both during testing.

## Rendering and feedback

Draw: - Raw samples/line in one style. - Processed path in a visually
distinct style. - Start point and endpoint, or a start marker for a
closed loop. - Optional current look-ahead target when the follower is
active. - Clear status: Editing, Recording, Preview, or Following. - A
visible track-reference toggle state.

Use existing OpenGL rendering facilities. Do not introduce a GUI
framework solely for this stage unless the project already uses one or
the current architecture requires it.

The blank mode should hide the track reference, not the path feedback,
editor boundary, or necessary instructions. It should still use the same
world bounds so the result can be compared with track-visible mode.

## File organization

Possible files, adapted to repository conventions:

``` text
PathEditor.h / PathEditor.cpp
PathProcessor.h / PathProcessor.cpp
Path.h / Path.cpp
PathEditorRenderer.h / PathEditorRenderer.cpp  # only if useful in this codebase
```

The editor should call `PathProcessor` and then hand a `Path` to the
follower. Avoid direct dependencies from `PathEditor` to the `Car`
class.

## Testing

### Coordinate tests

-   Screen-to-world mapping at corners and center.
-   Correct vertical plane.
-   Track-visible and blank modes map identically.
-   Window resize and camera changes.
-   Invalid ray-plane intersection.

### Recording tests

-   Single click / too-short stroke.
-   Short and long strokes.
-   Fast mouse movement with sparse events.
-   Slow mouse movement with dense events.
-   Start/stop repeatedly.
-   Clear while editing and after preview.
-   Redraw after invalid path.
-   Switch modes without stale samples.

### Processing tests

-   Duplicate points.
-   Non-finite values.
-   Resampling spacing.
-   Smoothing on straight lines and sharp turns.
-   Open path endpoints.
-   Explicitly closed loop.
-   Total path length and cumulative distance.

### End-to-end tests

1.  Draw a simple straight line and confirm it.
2.  Confirm the car follows it.
3.  Draw an oval around the track and complete laps.
4.  Toggle track reference and verify the path does not move.
5.  Stop following, edit, and confirm a new path.
6.  Verify WASD still works after leaving editor mode.
7.  Ensure a rejected path never replaces the last valid path unless the
    user explicitly chooses to discard it.

## Experiment support

Record, at minimum: - Input method = mouse. - Track reference
visible/hidden. - Raw sample count. - Processed point count and path
length. - Time spent drawing and number of redraws, if feasible. - Trial
ID and follower configuration.

Do not infer usability from path accuracy alone. A user might draw an
accurate path slowly or a poor path quickly; record both process and
outcome measures.

## Milestones and acceptance criteria

### Milestone 1: coordinate mapping and overlay

-   Mouse position maps to world space correctly.
-   Path points can be visualized.

### Milestone 2: edit workflow

-   Record, stop, clear, preview, and redraw work.
-   Track-visible and blank modes share a stable mapping.

### Milestone 3: processing and validation

-   Raw and processed paths are both available.
-   Invalid paths produce understandable feedback.
-   Closed/open path semantics are explicit.

### Milestone 4: follower integration

-   A confirmed mouse path can be handed to Stage 1.
-   The car can be stopped and the path redesigned without restarting
    the game.
-   Existing WASD, collisions, checkpoints, and scoring still work.

Acceptance criterion: a user can draw, inspect, clear, replace, and
confirm a path, and the existing path follower follows it through the
current physics system.

## Coding-agent prompt

> Implement Stage 2 only, building on the existing Stage 1 path
> representation and follower. First inspect the project's
> window/input/camera/rendering architecture and recommend either an
> in-game overlay or a separate editor based on the smallest reliable
> implementation. Create a mouse editor with explicit
> edit/record/preview/follow states, track-visible and blank reference
> modes, clear/redraw/confirm/stop actions, and correct screen-to-world
> mapping. Preserve raw samples and produce a validated processed path
> using dependency-free resampling and optional conservative smoothing.
> The track visibility toggle must not change coordinate mapping. Do not
> modify the car physics or add third-party dependencies. Integrate the
> confirmed path with the existing follower, keep WASD functional, build
> and test incrementally, and document controls, mapping assumptions,
> and changed files.
