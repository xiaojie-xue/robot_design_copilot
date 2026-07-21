# Engineering Validation Method

Every public engineering capability requires multiple forms of evidence. Unit tests alone do not establish engineering correctness.

## Validation layers

1. **Input invariants:** units, dimensions, frames, finite values, rotations, inertias, limits, and topology.
2. **Analytic models:** one-link, two-link, or other hand-verifiable cases.
3. **Independent checks:** finite-difference Jacobians, FK checks for IK, independent dynamics references, and URDF round trips.
4. **Golden cases:** versioned inputs, expected outputs, tolerances, and sources in `tests/golden/`.
5. **Property tests:** valid rigid transforms, FK–IK–FK consistency, inertia properties, and collision-distance consistency.
6. **Platform regression:** identical cases on supported platforms with compiler and dependency versions recorded.
7. **Product E2E:** `DesignSpec` through analysis, UI, persistence, and export without losing units, versions, warnings, or stale state.

## Minimum golden set

- Nominal seven-axis configuration
- Joint-limit configuration
- Singular or near-singular configuration
- Multiple payloads and gravity directions
- Solvable and unsolvable IK requests
- Colliding and collision-free configurations

## Correctness-critical defects

G2 cannot pass with any of these open:

- Incorrect unit or frame interpretation
- Non-finite or unconverged output reported as success
- Unreported hard-constraint violation
- Non-reproducible versioned input
- Result missing input or algorithm version
- Unexplained deviation from an independent reference

Each golden record identifies its source. Each CI report identifies its commit. Each persisted `AnalysisResult` identifies its specification revision, candidate revision, engine version, algorithm configuration, and catalog version when applicable.
