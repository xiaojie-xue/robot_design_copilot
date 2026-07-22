# M3 — AI Assistance

Status: Planned

Required gate: [G3 — AI Safety](../gates/G3-ai-safety.md)

## Outcome

AI can refine requirements and explain deterministic results through reviewable,
permissioned interactions.

## In scope

- DeepSeek, OpenAI-compatible, and Ollama provider interfaces
- Streaming, cancellation, and structured output
- Missing-requirement questions and `DesignSpecPatch` review
- Explanations grounded in versioned `AnalysisResult` data
- Provider/model/prompt records, permissions, and redaction

## Out of scope

- Direct AI writes to accepted specifications
- AI-generated engineering results
- Arbitrary filesystem or shell access

## Deliverables

- Provider adapters and mock provider
- Patch validation and per-field approval flow
- Grounded explanation flow
- Permission, credential, and redaction tests

## Demonstration

Review AI questions and a proposed patch, accept selected fields, rerun the
deterministic engine, and inspect an explanation linked to those results.

## Completion criteria

- [ ] All engineering functions work without an AI provider.
- [ ] Invalid model output cannot modify project state.
- [ ] Every accepted field change is explicitly approved.
- [ ] Numerical explanations cite deterministic result fields.
- [ ] Credentials do not appear in logs, projects, or exports.
- [ ] G3 passes.
