# Robot Design Copilot

[English](README.md) | [简体中文](README_zh.md)

> Describe the robot you need. Refine it with AI. Validate it with engineering tools.

Robot Design Copilot is an open-source, cross-platform assistant for early-stage robot design. It turns natural-language requirements into structured specifications, evaluates candidate designs, and recommends joint components and existing robots.

Initial design targets:

- 7-axis robotic arms
- Wheeled dual-arm robots

> [!NOTE]
> The project is currently in the design and early development stage. No installable release is available yet.

## Core idea

**AI proposes -> engineering tools validate -> the user decides.**

The language model handles requirement capture, follow-up questions, and explanations. Deterministic tools handle geometry, kinematics, dynamics, optimization, and component compatibility. The versioned `DesignSpec`—not the chat history—is the source of truth.

## Planned features

- Conversational requirement refinement with editable engineering parameters
- Workspace, singularity, collision, speed, and joint-load analysis
- Motor, reducer, drive, brake, and bearing recommendations
- Link-length and component-combination optimization
- Explainable recommendations with assumptions, margins, and data sources
- Existing robot matching against the same requirements
- Interactive 3D preview for joints, workspace, constraints, and trajectories
- JSON, BOM, report, URDF, MJCF, glTF, and STEP/STL export
- Windows, macOS, and Linux desktop releases

## Workflow

1. Choose a robot type and describe the task.
2. Refine missing requirements with the AI assistant.
3. Review the generated `DesignSpec`.
4. Generate and validate design candidates.
5. Compare components, designs, and existing robots.
6. Preview in 3D and export the result.

## Proposed stack

| Layer | Technology |
| --- | --- |
| Desktop | Tauri 2 / Rust |
| UI and 3D | React, TypeScript, Three.js |
| Engineering core | C++20, Pinocchio, Eigen, Ceres |
| Local API and schemas | Structured JSON over framed IPC |
| CAD and simulation | CadQuery, MuJoCo |
| Storage | SQLite |

DeepSeek will be the default provider preset. Users will supply their own key or connect a custom OpenAI-compatible API or local Ollama model. Credentials will be stored securely and never embedded in releases.

## MVP

- [ ] Requirement and `DesignSpec` schemas
- [ ] DeepSeek and custom model providers
- [ ] Parametric 7-axis arm
- [ ] Kinematics, workspace, and basic joint-load calculations
- [ ] Curated motor and reducer catalog
- [ ] Explainable component recommendations
- [ ] Simplified interactive 3D preview
- [ ] JSON, BOM, report, and URDF export

## Safety

Robot Design Copilot is an engineering assistance tool, not a certification authority. Designs must be reviewed by qualified engineers before procurement, manufacturing, or operation.

## Development roadmap

See the [project roadmap](docs/project/roadmap.md), [architecture overview](docs/architecture/overview.md), and [gate reviews](docs/project/gates/README.md).

Development retains the original M0 platform baseline. Its narrowed G0 review
confirms the architecture and three-platform build direction without requiring
finished desktop packaging. The first design calculation derives
link lengths from workspace requirements and per-joint requirements from the
resulting geometry, payload, and motion limits. The chain is available through
the direct `robot-engine-design` executable and framed `design.calculate`
request; see the [calculation guide](docs/engineering/design-calculation.md). The current sidecar implements
bounded IPC framing and envelope validation,
structured errors, and an `engine.health` request. Its C++ request session runs
concurrent jobs, serializes progress and terminal envelopes, propagates
cooperative cancellation, and joins workers on shutdown. The Rust lifecycle client owns the isolated
engine process, matches concurrent requests, enforces timeouts, sends
cancellation, and handles crash, restart, and shutdown paths. See the
[development plan](docs/project/development-plan.md) for delivery order and the
current work breakdown. The first C++ engine slice uses CMake 3.22+ and a
C++20 compiler. Configure and build with the bundled `dev` preset, then run the
tests:

```shell
cmake --preset dev
cmake --build --preset dev
ctest --test-dir build/dev --output-on-failure
```

To keep every local test artifact inside this repository, use:

```shell
bash scripts/test-engine.sh
```

The script configures `build/core`, reuses the repository-local Conan toolchain
when available, runs the unit binaries directly, performs a
real framed sidecar health exchange, and checks the committed protocol JSON.
It also runs feasible/infeasible design cases, analytic dynamics references, error
paths, and reproducibility checks. Inspect the structured result directly with:

```shell
build/core/engine/robot-engine-design tests/golden/design_calculation/feasible-input.json
```

The robotics dependency set is installed into a repository-local Conan home:

```shell
bash scripts/setup-cpp-deps.sh
```

It pins Pinocchio 3.8.0, Ceres Solver 2.2.0, Eigen 3.4.1, and nlohmann/json
3.12.0. No Conan package cache or test artifact is written outside `build/`.

After dependency setup, build and test the first seven-axis Pinocchio slice:

```shell
bash scripts/test-robotics.sh
```

This repository-local test builds `build/robotics`, verifies the reference-arm
forward kinematics analytically, checks invalid protocol parameters, and runs
real framed `engine.health` and `kinematics.forward` exchanges through the
sidecar. The reference model is an M0 integration fixture, not a production
robot design.

The M0 desktop FK scaffold uses Tauri 2, React, and a minimum command surface.
It is reused during M5 product packaging. The webview can request engine health,
forward kinematics, or an engine restart, but it has no direct shell permission.
Install and verify the frontend without starting the engine:

```shell
corepack enable
pnpm install --frozen-lockfile --ignore-scripts
pnpm check
pnpm test
pnpm build
```

After `scripts/test-robotics.sh` has produced the C++ executable, stage it under
Tauri's target-triple filename and launch the desktop application in a normal
development environment:

```shell
python3 scripts/stage-sidecar.py
pnpm tauri dev
```

Icons are generated (not committed). Before the first desktop build, generate
them from a `1024x1024` source PNG or `generate_context!()` fails on the missing
`icon.png`:

```shell
pnpm tauri icon path/to/source.png
```

The last command launches the bundled engine sidecar. Do not run it in process
sandboxes that terminate applications when they create child processes.

## Contributing

The project is at an early stage. Contributions to schemas, verified calculations, component data, robotics algorithms, 3D visualization, cross-platform packaging, and documentation are welcome.

## License

Licensed under the [Apache License 2.0](LICENSE).
