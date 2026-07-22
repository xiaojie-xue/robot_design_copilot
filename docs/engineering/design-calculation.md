# Design Calculation

This is an executable C++20 calculation chain for the parametric seven-axis arm.
It accepts one normalized JSON input, finds bounded link lengths, verifies every
requested pose with FK/IK, and uses the verified geometry directly to calculate
joint and motor-side requirements.

## Run the complete chain

From the repository root:

```shell
bash scripts/test-engine.sh
build/core/engine/robot-engine-design tests/golden/design_calculation/feasible-input.json
build/core/engine/robot-engine-design tests/golden/design_calculation/infeasible-input.json
```

`robot-engine-design` also accepts the same JSON on standard input. The framed
sidecar exposes the calculation as `design.calculate`, with the input used
directly as the request `params` object.

The feasible command returns `status: success`, complete geometry and workspace
evidence, case-level dynamics, and seven joint requirements. The negative
fixture returns `status: infeasible` and never emits dynamics. Invalid,
under-specified, non-finite, and iteration-limited inputs return
`invalid_input` or `non_converged`; neither can carry sizing results.

## Contracts

- [`design-input.schema.json`](../../protocol/design-input.schema.json) defines
  the calculation input.
- [`design-result.schema.json`](../../protocol/design-result.schema.json)
  defines the calculation result.
- [`feasible-input.json`](../../tests/golden/design_calculation/feasible-input.json),
  [`infeasible-input.json`](../../tests/golden/design_calculation/infeasible-input.json), and
  [`feasible-expected.json`](../../tests/golden/design_calculation/feasible-expected.json) are
  the committed golden records.

The engine performs semantic checks beyond JSON Schema: ordered and consistent
link/joint IDs, `min <= max`, distinct frames, unit quaternions and direction
vectors, positive tolerances, finite values, referenced target IDs, explicit
mass properties, seven transmissions, safety factors, and bounded solver
settings.

## Model and solver

The model has seven revolute axes in the deterministic `Z-Y-Z-Y-Z-Y-Z`
sequence. The three designed links are +X offsets after joints 2, 4, and 6.
The input declares base height, all position limits, and lower/upper bounds for
the upper arm, forearm, and wrist.

The geometry solver minimizes a common normalized position within the three
link bounds. It performs deterministic bisection and accepts a candidate only
when damped-least-squares IK followed by FK verifies the position and
orientation of every target inside that target's declared tolerances. This is a
bounded deterministic candidate search, not a claim of global morphology
optimality. The result records the bracket width, iteration count, objective,
residuals, IK terminations, and active bounds.

## Joint requirements

Each dynamics evaluation case references a verified workspace posture and
declares TCP linear/angular velocity and acceleration directions. A geometric
Jacobian maps the declared TCP maxima to joint velocity and acceleration.
Singular or ill-conditioned mappings are reported as warnings and retain their
case IDs. The result records the damping, the position/orientation scaling, and
the condition-number warning threshold used by that mapping.

The calculation constructs a posture-specific mass matrix from declared link
mass, link COM fraction, scalar link rotational inertia, payload mass, payload
COM, and payload diagonal inertia. It reports unsafed gravity and inertial
torque separately. Required torque is the conservative sum

```text
gravity_factor * abs(gravity_torque)
  + dynamic_factor * abs(inertial_torque)
```

Mechanical power is required torque times absolute joint speed. Motor-side
speed, torque, and power use each declared ratio and efficiency. Every peak
retains the evaluation case that produced it.

This posture model neglects `Jdot*qdot`; that limitation is explicit in the
result. It is suitable for the current sizing proof and must be replaced or bounded
by time-parameterized trajectory dynamics before later manufacturing decisions.

## Validation evidence

`robot-engine-design-tests` covers:

- feasible FK/IK coverage and geometry-to-dynamics integration;
- a maximum-reach infeasible case that fails closed;
- missing mass properties and evaluation cases, non-finite values, and
  optimizer iteration exhaustion;
- exact reproducibility for a fixed input revision;
- an independent straight-chain minimum-length reference;
- independent static moment-arm gravity torques for joints 2, 4, and 6;
- analytic reconstruction of the requested TCP twist and acceleration from the
  reported joint values;
- torque-speed mechanical-power consistency.

The committed golden tolerances use the repository absolute-plus-relative rule.
The 10 µm geometry tolerance matches the fixture's target acceptance threshold;
the 0.02 N·m plus 0.3% torque tolerance covers the small accepted pose and link
residual without hiding a sizing-significant error. Cross-platform structured
comparison remains the final pending G1 review item.
