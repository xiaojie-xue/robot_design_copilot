# Numerical Tolerances

Floating-point tests must not require exact equality or use unexplained thresholds.

## Comparison rule

Use absolute and relative tolerance together:

```text
abs(actual - expected) <= abs_tol + rel_tol * abs(expected)
```

Vector and matrix tests state whether they compare elements or norms. Rotation tests compare relative rotation angle, not quaternion components.

## Required documentation

Each algorithm or golden case records:

- Reference source and precision
- Algorithm and termination conditions
- Input scale and units
- Absolute and relative tolerances
- Engineering justification
- Known platform differences

Keep tolerances beside the test or in a typed test configuration. Do not use undocumented numeric literals.

## Iterative algorithms

IK and optimization results record convergence status, termination reason, iterations, final residual, constraint violation, and limit. Reaching an iteration or time limit is not success unless all published acceptance conditions pass.

## Golden-result changes

Update golden results only in a dedicated review PR. The PR must state why the old value is wrong, what changed, the old/new error comparison, and whether design conclusions change. Test commands must never overwrite golden files.

## Platform comparison

Compare structured results across supported platforms. Require engineering equivalence within approved tolerances, not bitwise identity. Investigate compiler, dependency, or code-path differences before changing a tolerance.
