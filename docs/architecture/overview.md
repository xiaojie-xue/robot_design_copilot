# Architecture Overview

Robot Design Copilot is a local-first desktop application. AI refines requirements and explains results. The C++ engine produces deterministic calculations. The user approves requirements and design decisions.

## Runtime

```mermaid
flowchart LR
    UI["React / Three.js"]
    Rust["Tauri / Rust"]
    Engine["C++ Engineering Engine"]
    DB["SQLite"]
    Files["Project files"]
    LLM["DeepSeek / compatible API / Ollama"]
    Catalog["Versioned catalogs"]

    UI -->|Tauri commands| Rust
    Rust -->|length-prefixed IPC| Engine
    Rust --> DB
    Rust --> Files
    Rust --> LLM
    Engine --> Catalog
```

## Data flow

```text
User description
  -> DesignSpecPatch
  -> schema validation
  -> user approval
  -> DesignSpec revision
  -> CandidateDesign
  -> AnalysisResult
  -> Recommendation
  -> DecisionRecord
  -> ExportArtifact
```

## Boundaries

1. React does not access SQLite, the filesystem, or the C++ engine directly.
2. Rust does not duplicate C++ engineering algorithms.
3. The C++ engine does not store API keys or call language models.
4. AI cannot write an accepted `DesignSpec` directly.
5. Cross-language interfaces use versioned data contracts, not private language types.
6. The engineering core uses SI units.

## M0 desktop command boundary

The first desktop slice exposes only `engine_health`, `forward_kinematics`, and
`restart_engine` to React. Tauri owns the `robot-engine-client`, resolves the
bundled `robot-engine-cli-$TARGET_TRIPLE` external binary, and maps transport or
protocol failures into structured command errors. The webview receives no shell
plugin capability, so it cannot spawn arbitrary processes or bypass the Rust
lifecycle owner.

See [ADRs](../adr/README.md) for decisions and the [roadmap](../project/roadmap.md) for delivery order.
