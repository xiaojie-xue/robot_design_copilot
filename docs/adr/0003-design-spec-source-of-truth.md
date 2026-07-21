# ADR-0003: DesignSpec as the Requirement Source of Truth

Status: Accepted  
Date: 2026-07-21

## Context

Natural-language conversations contain ambiguity, stale statements, and model assumptions. Candidate designs and calculated results also have different lifecycles from user requirements.

## Decision

The versioned `DesignSpec` is the only accepted requirement source. Keep these records separate:

- `DesignSpec`: accepted user requirements.
- `CandidateDesign`: a proposed design.
- `AnalysisResult`: deterministic tool output.
- `Recommendation`: a constraint-based recommendation.
- `DecisionRecord`: a user acceptance or rejection.

AI may produce only a `DesignSpecPatch`. A patch creates a new revision only after schema validation, visible diff review, and user approval.

Each analysis result records the specification revision, candidate revision, engine version, algorithm configuration, catalog version when applicable, units, assumptions, and warnings. Input changes retain old results but mark them stale.

## Consequences

- Engineering results are reproducible and auditable.
- Users can review every AI-generated change.
- The data model requires explicit references and migrations.
- The UI must distinguish requirements, candidates, results, recommendations, and decisions.

## Alternatives considered

- Chat history as source of truth: rejected because it cannot define one current requirement set.
- Results embedded in `DesignSpec`: rejected because it mixes inputs with stale outputs.
- Direct AI updates: rejected because hidden assumptions would bypass user review.
