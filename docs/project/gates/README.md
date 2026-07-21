# Gate Reviews

A gate is an evidence-based go/no-go decision. Feature completion alone does not prove correctness, traceability, or release readiness.

## Decisions

| Decision | Definition |
| --- | --- |
| Passed | Every mandatory exit criterion is met. |
| Passed with conditions | Only non-critical work is deferred to a named issue and deadline milestone. |
| Failed | At least one mandatory criterion is not met. |
| Pending | Review has not finished. |

Correctness, data provenance, credential exposure, and user-safety defects cannot pass with conditions.

## Process

1. Complete the milestone implementation and tests.
2. Open a pull request named for the gate.
3. Run required CI against the review commit.
4. Review evidence and known limitations.
5. Append the decision, commit, reviewers, and evidence to the gate record.
6. Update the roadmap after the review PR merges.

Commit schemas, golden cases, validation methods, ADRs, and review decisions. Keep binaries, full test reports, sanitizer reports, benchmarks, and SBOMs in CI artifacts or releases. Never overwrite prior review records.
