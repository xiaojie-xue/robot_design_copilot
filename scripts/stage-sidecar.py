#!/usr/bin/env python3

import argparse
import platform
import shutil
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def host_target() -> str:
    system = platform.system()
    machine = platform.machine().lower()
    architecture = {
        "arm64": "aarch64",
        "aarch64": "aarch64",
        "amd64": "x86_64",
        "x86_64": "x86_64",
    }.get(machine)
    if architecture is None:
        raise SystemExit(f"unsupported host architecture: {machine}")
    if system == "Darwin":
        return f"{architecture}-apple-darwin"
    if system == "Linux":
        return f"{architecture}-unknown-linux-gnu"
    if system == "Windows":
        return f"{architecture}-pc-windows-msvc"
    raise SystemExit(f"unsupported host platform: {system}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Stage robot-engine-cli with Tauri's target-triple suffix"
    )
    parser.add_argument("--target", default=host_target())
    parser.add_argument("--source", type=Path)
    arguments = parser.parse_args()

    extension = ".exe" if "windows" in arguments.target else ""
    source = arguments.source or (
        ROOT / "build" / "dev" / "engine" / f"robot-engine-cli{extension}"
    )
    if not source.is_file():
        raise SystemExit(
            f"engine binary not found: {source}\n"
            "build the robotics engine first or pass --source"
        )

    destination = (
        ROOT
        / "src-tauri"
        / "binaries"
        / f"robot-engine-cli-{arguments.target}{extension}"
    )
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    destination.chmod(destination.stat().st_mode | 0o111)
    print(f"staged {destination.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
