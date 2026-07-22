#!/usr/bin/env python3

import json
import pathlib
import subprocess
import sys


def run(engine: pathlib.Path, fixture: pathlib.Path) -> dict:
    completed = subprocess.run(
        [engine, fixture], check=True, capture_output=True, text=True, timeout=20
    )
    assert completed.stderr == ""
    return json.loads(completed.stdout)


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit(
            "usage: design_calculation_cli_integration_test.py ENGINE FIXTURE_DIR"
        )
    engine = pathlib.Path(sys.argv[1])
    fixture_dir = pathlib.Path(sys.argv[2])

    feasible = run(engine, fixture_dir / "feasible-input.json")
    assert feasible["status"] == "success"
    assert feasible["workspace"]["coverage"] == 1.0
    assert len(feasible["dynamics"]["joint_requirements"]) == 7

    infeasible = run(engine, fixture_dir / "infeasible-input.json")
    assert infeasible["status"] == "infeasible"
    assert "dynamics" not in infeasible
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
