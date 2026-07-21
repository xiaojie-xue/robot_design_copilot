#!/usr/bin/env python3

import json
import os
import pathlib
import struct
import sys


def read_frame() -> dict | None:
    header = sys.stdin.buffer.read(4)
    if header == b"":
        return None
    if len(header) != 4:
        raise RuntimeError("truncated header")
    (length,) = struct.unpack(">I", header)
    payload = sys.stdin.buffer.read(length)
    if len(payload) != length:
        raise RuntimeError("truncated payload")
    return json.loads(payload)


def write_frame(message: dict) -> None:
    payload = json.dumps(message, separators=(",", ":")).encode("utf-8")
    sys.stdout.buffer.write(struct.pack(">I", len(payload)) + payload)
    sys.stdout.buffer.flush()


def mark(path: str, value: str = "ok") -> None:
    pathlib.Path(path).write_text(value, encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: fake_engine.py MODE MARKER")
    mode, marker = sys.argv[1:]

    if mode == "crash-after-frame":
        read_frame()
        os._exit(42)

    if mode == "cancel-after-timeout":
        request = read_frame()
        cancel = read_frame()
        if request is None or cancel is None:
            return 2
        if cancel.get("type") != "cancel":
            return 3
        if cancel.get("request_id") != request.get("request_id"):
            return 4
        mark(marker, cancel["request_id"])
        return 0

    if mode == "wait-eof":
        mark(marker + ".started", str(os.getpid()))
        while read_frame() is not None:
            pass
        mark(marker + ".stopped")
        return 0

    if mode == "progress-response":
        request = read_frame()
        if request is None:
            return 6
        request_id = request["request_id"]
        write_frame(
            {
                "protocol_version": 1,
                "type": "progress",
                "request_id": request_id,
                "progress": {"fraction": 0.5, "stage": "fixture"},
            }
        )
        write_frame(
            {
                "protocol_version": 1,
                "type": "response",
                "request_id": request_id,
                "engine_version": "fixture",
                "result": {"done": True},
            }
        )
        return 0

    if mode == "crash-once":
        request = read_frame()
        if request is None:
            return 7
        if not pathlib.Path(marker).exists():
            mark(marker, "first process crashed")
            os._exit(42)
        write_frame(
            {
                "protocol_version": 1,
                "type": "response",
                "request_id": request["request_id"],
                "engine_version": "fixture",
                "result": {"restarted": True},
            }
        )
        return 0

    return 5


if __name__ == "__main__":
    raise SystemExit(main())
