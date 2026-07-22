# G3 — AI Safety Gate

Status: Not reviewed

Applies after: [M3](../milestones/M3-ai-assistance.md)

Decision: Pending

## Purpose

Verify that AI assistance is reviewable, grounded, permissioned, and unable to
silently alter engineering truth or expose credentials.

## Required evidence

| Requirement | Evidence | Status |
| --- | --- | --- |
| Invalid model output cannot change state | Patch validation tests | Pending |
| Users approve accepted changes | Per-field approval tests | Pending |
| Explanations reference engine results | Grounding tests | Pending |
| Provider calls can be cancelled | Provider lifecycle tests | Pending |
| Credentials and sensitive fields are redacted | Security tests | Pending |
| Offline engineering still works | No-provider workflow test | Pending |

## Exit criteria

- [ ] AI cannot write accepted specifications without user approval.
- [ ] AI-generated numbers cannot masquerade as deterministic results.
- [ ] Provider, model, prompt, permissions, and accepted patches are traceable.
- [ ] API keys do not appear in logs, projects, diagnostics, or exports.
- [ ] Default CI passes with mock providers only.

## Review record

| Date | Commit | Reviewers | Decision | Conditions and evidence |
| --- | --- | --- | --- | --- |
| — | — | — | Pending | — |
