#!/usr/bin/env bash

set -euo pipefail

task_repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
task_build_dir="${task_repo_dir}/build/robotics"
task_generators_dir="${task_repo_dir}/build/conan/release/build/Release/generators"
task_toolchain="${task_generators_dir}/conan_toolchain.cmake"

cd "${task_repo_dir}"

if [[ ! -f "${task_toolchain}" ]]; then
  echo "Missing repository-local dependencies; run bash scripts/setup-cpp-deps.sh" >&2
  exit 1
fi

cmake -S . -B "${task_build_dir}" \
  -DCMAKE_TOOLCHAIN_FILE="${task_toolchain}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON \
  -DROBOT_ENGINE_ENABLE_ROBOTICS=ON \
  -DROBOT_ENGINE_FETCH_DEPENDENCIES=OFF
cmake --build "${task_build_dir}" --parallel

# The Conan run environment contains the repository-local Pinocchio dylibs.
source "${task_generators_dir}/conanrun.sh"

"${task_build_dir}/engine/robot-engine-frame-tests"
"${task_build_dir}/engine/robot-engine-protocol-tests"
"${task_build_dir}/engine/robot-engine-session-tests"
"${task_build_dir}/engine/robot-engine-kinematics-tests"
"${task_build_dir}/engine/robot-engine-design-tests"

python3 engine/tests/cli_pipe_fixture.py emit-health \
  | "${task_build_dir}/engine/robot-engine-cli" \
  | python3 engine/tests/cli_pipe_fixture.py verify-robotics-health

python3 engine/tests/cli_pipe_fixture.py emit-forward \
  | "${task_build_dir}/engine/robot-engine-cli" \
  | python3 engine/tests/cli_pipe_fixture.py verify-forward

python3 -m json.tool protocol/envelope.schema.json >/dev/null
for task_fixture in protocol/examples/*.json; do
  python3 -m json.tool "${task_fixture}" >/dev/null
done
python3 -m json.tool protocol/design-input.schema.json >/dev/null
python3 -m json.tool protocol/design-result.schema.json >/dev/null
for task_fixture in tests/golden/design_calculation/*.json; do
  python3 -m json.tool "${task_fixture}" >/dev/null
done

echo "All in-repository robotics tests passed"
