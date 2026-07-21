#!/usr/bin/env python3

import json
import struct
import sys


REQUEST_ID = "pipe-health"
FORWARD_REQUEST_ID = "pipe-fk-zero"


def write_frame(message: dict) -> None:
    payload = json.dumps(message, separators=(",", ":")).encode("utf-8")
    sys.stdout.buffer.write(struct.pack(">I", len(payload)) + payload)


def read_frame() -> dict:
    header = sys.stdin.buffer.read(4)
    if len(header) != 4:
        raise AssertionError(f"expected 4 header bytes, received {len(header)}")
    (length,) = struct.unpack(">I", header)
    payload = sys.stdin.buffer.read(length)
    if len(payload) != length:
        raise AssertionError(f"expected {length} payload bytes, received {len(payload)}")
    if sys.stdin.buffer.read() != b"":
        raise AssertionError("engine wrote bytes outside the response frame")
    return json.loads(payload)


def emit_health() -> None:
    write_frame(
        {
            "protocol_version": 1,
            "type": "request",
            "request_id": REQUEST_ID,
            "method": "engine.health",
            "params": {},
        }
    )


def emit_forward() -> None:
    write_frame(
        {
            "protocol_version": 1,
            "type": "request",
            "request_id": FORWARD_REQUEST_ID,
            "method": "kinematics.forward",
            "params": {"joint_positions_rad": [0.0] * 7},
        }
    )


def verify_health(robotics: bool = False) -> None:
    response = read_frame()
    assert response["type"] == "response"
    assert response["request_id"] == REQUEST_ID
    assert response["result"]["status"] == "ok"
    assert response["result"]["dependency_smoke_check"] is True
    expected_dependencies = {
        "eigen": "3.4.1",
        "nlohmann_json": "3.12.0",
    }
    expected_capabilities = ["engine.health"]
    if robotics:
        expected_dependencies.update(
            {
                "pinocchio": "3.8.0",
                "ceres": "2.2.0",
            }
        )
        expected_capabilities.append("kinematics.forward")
    assert response["result"]["dependencies"] == expected_dependencies
    assert response["result"]["capabilities"] == expected_capabilities
    print("Sidecar health pipe passed")


def verify_forward() -> None:
    response = read_frame()
    assert response["type"] == "response"
    assert response["request_id"] == FORWARD_REQUEST_ID
    result = response["result"]
    assert result["model_id"] == "reference_arm_7dof_v1"
    assert result["base_frame"] == "base"
    assert result["tool_frame"] == "tool0"
    assert result["joint_count"] == 7
    for actual, expected in zip(result["translation_m"], [1.26, 0.0, 0.54]):
        assert abs(actual - expected) < 1e-12
    for actual, expected in zip(result["orientation_xyzw"], [0.0, 0.0, 0.0, 1.0]):
        assert abs(actual - expected) < 1e-12
    print("Sidecar forward-kinematics pipe passed")


def main() -> int:
    actions = {
        "emit-health",
        "emit-forward",
        "verify-health",
        "verify-robotics-health",
        "verify-forward",
    }
    if len(sys.argv) != 2 or sys.argv[1] not in actions:
        raise SystemExit(
            "usage: cli_pipe_fixture.py "
            "{emit-health|emit-forward|verify-health|"
            "verify-robotics-health|verify-forward}"
        )
    if sys.argv[1] == "emit-health":
        emit_health()
    elif sys.argv[1] == "emit-forward":
        emit_forward()
    elif sys.argv[1] == "verify-health":
        verify_health()
    elif sys.argv[1] == "verify-forward":
        verify_forward()
    else:
        verify_health(robotics=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
