#!/usr/bin/env bash

set -euo pipefail

task_repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
task_rust_home="${task_repo_dir}/build/tools/rustup"
task_cargo_home="${task_repo_dir}/build/tools/cargo"
task_cargo_target="${task_repo_dir}/build/rust-target"
task_test_state="${task_repo_dir}/build/rust-test-state"
task_tmp_dir="${task_repo_dir}/build/tmp"
task_rust_version="1.97.1"
task_engine_binary="${task_repo_dir}/build/robotics/engine/robot-engine-cli"
task_generators_dir="${task_repo_dir}/build/conan/release/build/Release/generators"

cd "${task_repo_dir}"

if [[ ! -x "${task_cargo_home}/bin/cargo" ]]; then
  echo "Missing repository-local Rust toolchain; run bash scripts/setup-rust.sh" >&2
  exit 1
fi
if [[ ! -x "${task_engine_binary}" ]]; then
  echo "Missing robotics engine; run bash scripts/test-robotics.sh" >&2
  exit 1
fi

export RUSTUP_HOME="${task_rust_home}"
export CARGO_HOME="${task_cargo_home}"
export CARGO_TARGET_DIR="${task_cargo_target}"
export ROBOT_ENGINE_TEST_BINARY="${task_engine_binary}"
export ROBOT_ENGINE_TEST_PYTHON="$(command -v python3)"
export ROBOT_ENGINE_TEST_STATE_DIR="${task_test_state}"
export TMPDIR="${task_tmp_dir}"
export PATH="${task_cargo_home}/bin:${PATH}"

mkdir -p "${task_test_state}" "${task_tmp_dir}"

task_toolchain=""
for task_candidate in "${task_rust_version}" stable; do
  task_candidate_version="$(
    "${task_cargo_home}/bin/rustup" run "${task_candidate}" rustc --version 2>/dev/null || true
  )"
  if [[ "${task_candidate_version}" == "rustc ${task_rust_version} "* ]]; then
    task_toolchain="${task_candidate}"
    break
  fi
done
if [[ -z "${task_toolchain}" ]]; then
  echo "Missing repository-local Rust ${task_rust_version}; run bash scripts/setup-rust.sh" >&2
  exit 1
fi
export RUSTUP_TOOLCHAIN="${task_toolchain}"
echo "Testing with repository-local Rust toolchain: ${task_toolchain} (${task_rust_version})"

source "${task_generators_dir}/conanrun.sh"

find rust -type f -name '*.rs' \
  -exec "${task_cargo_home}/bin/rustfmt" --edition 2024 --check {} +
"${task_cargo_home}/bin/cargo" clippy \
  --workspace --all-targets --all-features --locked -- -D warnings
"${task_cargo_home}/bin/cargo" test --workspace --all-features --locked

echo "All in-repository Rust engine-client tests passed"
