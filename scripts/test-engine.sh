#!/usr/bin/env bash

set -euo pipefail

task_repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
task_build_dir="${task_repo_dir}/build/core"
task_toolchain="${task_repo_dir}/build/conan/release/build/Release/generators/conan_toolchain.cmake"
task_build_type="Debug"
task_cmake_args=()

cd "${task_repo_dir}"

if [[ -f "${task_toolchain}" ]]; then
  task_build_type="Release"
  task_cmake_args+=(
    -DCMAKE_TOOLCHAIN_FILE="${task_toolchain}"
    -DROBOT_ENGINE_FETCH_DEPENDENCIES=OFF
  )
fi

task_cmake_args+=(
  -DBUILD_TESTING=ON
  -DCMAKE_BUILD_TYPE="${task_build_type}"
  -DROBOT_ENGINE_ENABLE_ROBOTICS=OFF
)

cmake -S . -B "${task_build_dir}" "${task_cmake_args[@]}"
cmake --build "${task_build_dir}" --parallel

"${task_build_dir}/engine/robot-engine-frame-tests"
"${task_build_dir}/engine/robot-engine-protocol-tests"

python3 engine/tests/cli_pipe_fixture.py emit-health \
  | "${task_build_dir}/engine/robot-engine-cli" \
  | python3 engine/tests/cli_pipe_fixture.py verify-health

python3 -m json.tool protocol/v1/envelope.schema.json >/dev/null
for task_fixture in protocol/v1/examples/*.json; do
  python3 -m json.tool "${task_fixture}" >/dev/null
done

echo "All in-repository engine tests passed"
