# Engine Protocol

The engine protocol carries UTF-8 JSON messages between the Rust application
process and the C++ engine sidecar.

## Framing

Each message is encoded as:

1. A four-byte unsigned payload length in network byte order (big endian).
2. Exactly that many bytes of UTF-8 JSON.

The default maximum payload is 16 MiB. A zero-length message, truncated header,
truncated payload, oversized payload, invalid UTF-8, or invalid JSON is a
protocol error. The first four conditions are enforced by the C++ transport
layer; UTF-8, JSON, and envelope validation belong to the semantic layer.

Standard output is reserved for framed messages. Human-readable and structured
diagnostic logs go to standard error.

## Envelopes

[`envelope.schema.json`](envelope.schema.json) defines requests, cancellations,
progress, successful responses, and errors. Correlated messages carry the same
non-empty `request_id`. An error caused before a valid request ID can be read uses a null
`request_id` and cannot be matched to an in-flight request.

Request IDs are 1-128 byte ASCII identifiers matching
`[A-Za-z0-9][A-Za-z0-9._:-]*`. Method names are lowercase ASCII identifiers up
to 128 bytes, such as `engine.health`. Unknown envelope fields are rejected so
schema drift is visible instead of silently ignored.

Cancellation is best-effort. A request completes with either a response or an
error; progress events do not complete it. Unsupported methods produce
structured errors rather than terminating the sidecar.

The C++ request session accepts frames on one reader thread and executes valid
request IDs as independent jobs. Complete output envelopes are serialized, so
their bytes never interleave even when responses finish out of input order. A
duplicate in-flight ID completes with `duplicate_request`. Long-running handlers
receive a stop token and a progress callback; after a valid matching `cancel`,
cooperative work completes with `request_cancelled`. A valid cancellation has no
separate acknowledgement, and cancellation of an unknown or already completed
request is ignored. A normal input EOF drains accepted work and joins every
remaining worker before the sidecar exits; framing/output failure requests
cancellation before joining.

`robot-engine-cli` implements semantic validation, structured errors, request
correlation, `engine.health`, and `design.calculate`. The latter uses a
complete design input document as the request `params` and returns the design
result in the response `result`; see the schemas beside this document.
Calculation outcomes are expressed as
`success`, `infeasible`, `invalid_input`, or `non_converged` inside the result
rather than as transport errors.

Robotics-enabled builds also expose `kinematics.forward` with the following
method-specific contract:

- `params.joint_positions_rad` is an array of exactly seven finite numbers.
- The input unit is radians and values are ordered from `joint_1` to `joint_7`.
- The response reports `base` to `tool0` translation in metres and orientation
  as a quaternion in explicit `[x, y, z, w]` order.
- Invalid method parameters return the stable `invalid_params` error code.

The M0 reference model uses alternating Z/Y revolute axes. At zero position,
`base_T_tool0` has translation `[1.26, 0.0, 0.54]` metres and identity
orientation. It is a deterministic integration and validation fixture rather
than a production arm definition. See the committed `forward-request.json` and
`forward-response.json` examples.

`robot-engine-transport-echo` remains a framing-only diagnostic executable.
The Rust `engine-client` owns request matching and validates complete inbound
response, progress, and error envelopes against these field and value
constraints. It sends best-effort cancellation after a timeout. Additional
engineering methods will use the C++ session's cooperative progress and
cancellation context in subsequent milestones.
