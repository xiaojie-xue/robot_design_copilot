#!/usr/bin/env python3

import json
import struct
import subprocess
import sys


def frame(message: dict) -> bytes:
    payload = json.dumps(message, separators=(",", ":")).encode("utf-8")
    return struct.pack(">I", len(payload)) + payload


def read_frame(stream) -> dict:
    header = stream.read(4)
    if len(header) != 4:
        raise AssertionError(f"expected 4 header bytes, received {len(header)}")
    (length,) = struct.unpack(">I", header)
    payload = stream.read(length)
    if len(payload) != length:
        raise AssertionError(f"expected {length} payload bytes, received {len(payload)}")
    return json.loads(payload)


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: cli_integration_test.py PATH_TO_ENGINE")

    process = subprocess.Popen(
        [sys.argv[1]],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert process.stdin is not None
    assert process.stdout is not None
    assert process.stderr is not None

    process.stdin.write(
        frame(
            {
                "type": "request",
                "request_id": "integration-health",
                "method": "engine.health",
                "params": {},
            }
        )
    )
    process.stdin.flush()
    health = read_frame(process.stdout)
    assert health["type"] == "response"
    assert health["request_id"] == "integration-health"
    assert health["result"]["status"] == "ok"

    process.stdin.write(
        frame(
            {
                "type": "request",
                "request_id": "integration-missing",
                "method": "engine.missing",
                "params": {},
            }
        )
    )
    process.stdin.flush()
    missing = read_frame(process.stdout)
    assert missing["type"] == "error"
    assert missing["request_id"] == "integration-missing"
    assert missing["error"]["code"] == "method_not_found"

    process.stdin.close()
    exit_code = process.wait(timeout=5)
    stderr = process.stderr.read()
    assert exit_code == 0
    assert stderr == b"", f"unexpected stderr output: {stderr!r}"
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
