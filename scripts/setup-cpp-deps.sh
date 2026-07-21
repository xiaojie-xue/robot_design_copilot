#!/usr/bin/env bash

set -euo pipefail

task_repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
task_venv_dir="${task_repo_dir}/build/tools/conan"
task_conan_home="${task_repo_dir}/build/conan/home"
task_output_dir="${task_repo_dir}/build/conan/release"
task_host_profile="default"
task_lockfile="${task_repo_dir}/conan.lock"
task_lock_args=(--lockfile-out "${task_lockfile}")

cd "${task_repo_dir}"

export PIP_CACHE_DIR="${task_repo_dir}/build/tools/pip-cache"
python3 -m venv "${task_venv_dir}"
"${task_venv_dir}/bin/python" -m pip install \
  --disable-pip-version-check \
  --requirement tools/requirements-conan.txt

export CONAN_HOME="${task_conan_home}"
"${task_venv_dir}/bin/conan" profile detect --force

# ConanCenter currently publishes the selected Pinocchio/Ceres graph for
# macOS arm64 with Apple Clang 17. libc++ keeps that binary graph compatible
# with the newer Apple Clang used to compile this application's own targets.
if [[ "$(uname -s)" == "Darwin" && "$(uname -m)" == "arm64" ]]; then
  task_host_profile="${task_repo_dir}/profiles/conan/macos-arm64-apple-clang17"
fi

if [[ -f "${task_lockfile}" ]]; then
  task_lock_args+=(--lockfile "${task_lockfile}")
fi

"${task_venv_dir}/bin/conan" install . \
  --output-folder "${task_output_dir}" \
  --profile:host "${task_host_profile}" \
  --profile:build default \
  --build missing \
  "${task_lock_args[@]}"

echo "C++ dependencies are available under build/conan"
