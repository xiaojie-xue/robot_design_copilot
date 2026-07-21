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
| Engineering core | Python, Pinocchio, NumPy, SciPy |
| Local API and schemas | FastAPI, Pydantic |
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

## Contributing

The project is at an early stage. Contributions to schemas, verified calculations, component data, robotics algorithms, 3D visualization, cross-platform packaging, and documentation are welcome.

## License

Licensed under the [Apache License 2.0](LICENSE).
