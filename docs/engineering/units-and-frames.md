# Units and Coordinate Frames

This specification applies to schemas, Rust, C++, persistence, tests, and exports.

## Units

The engineering core and persisted normalized models use SI units.

| Quantity | Unit | Example field |
| --- | --- | --- |
| Length | metre | `link_length_m` |
| Mass | kilogram | `payload_kg` |
| Time | second | `duration_s` |
| Angle | radian | `joint_position_rad` |
| Angular velocity | radian/second | `joint_velocity_rad_s` |
| Force | newton | `force_n` |
| Torque | newton metre | `torque_nm` |
| Power | watt | `power_w` |
| Voltage | volt | `voltage_v` |
| Current | ampere | `current_a` |
| Temperature | kelvin | `temperature_k` |

The UI may display millimetres, degrees, rpm, or Celsius, but it converts values at the UI boundary and always displays the unit. A field may omit a unit suffix only when it is dimensionless.

## Frames

Use right-handed frames. Standard IDs are `world`, `base`, stable joint/link frame IDs, and an explicit tool frame such as `tool0`.

Every pose identifies its parent and child frame. Gravity is explicit input or versioned configuration; it is not hidden global state.

## Rotation

- The protocol uses quaternions or homogeneous transforms as authoritative values.
- Serialized quaternions use `xyzw` order and an explicit name such as `orientation_xyzw`.
- The engine rejects non-finite or zero-norm quaternions and normalizes valid input.
- Transform names state direction; `base_T_tool` maps tool coordinates into base coordinates.

## Matrices

Serialized matrices define rows, columns, storage order, units, and frames. Eigen memory layout is not a public protocol.

## Missing values

Represent unknown values with an absent field or explicit state. Do not use zero, `NaN`, an empty string, or a sentinel magnitude. Persisted numeric results must be finite; non-finite calculations become structured failures.
