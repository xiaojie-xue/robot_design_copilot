# M6 — AI Design Copilot

Status: Planned  
Required gate: None

## Outcome

AI identifies missing requirements, proposes reviewable specification patches, and explains tool-generated results.

## In scope

- DeepSeek, OpenAI-compatible, and Ollama providers
- Streaming and cancellation
- Structured output
- `DesignSpecPatch` review and approval
- Missing-requirement questions
- Explanations grounded in `AnalysisResult`
- Provider, prompt, and model version records
- Tool permissions and log redaction

## Out of scope

- Direct AI writes to accepted `DesignSpec`
- AI-generated engineering results
- Unapproved purchase or manufacturing decisions
- Arbitrary filesystem or shell access

## Deliverables

- Rust model-provider interface and adapters
- Patch validation and approval workflow
- Mock-provider tests
- Chat and diff-review UI
- AI permission specification

## Demonstration

Describe a task, review AI questions and a proposed patch, accept selected fields, create a revision, rerun the engine, and receive an explanation linked to tool results.

## Completion criteria

- [ ] All engineering functions work without an AI provider.
- [ ] Invalid model output cannot modify project state.
- [ ] Users approve or reject each proposed field change.
- [ ] Numerical explanations reference deterministic results.
- [ ] API keys do not appear in logs, projects, or exports.
- [ ] Default CI uses mock providers only.
