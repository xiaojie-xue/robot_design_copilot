#!/usr/bin/env bash

set -euo pipefail

task_repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
task_bootstrap="${task_repo_dir}/build/tools/rust-bootstrap/rustup-init"
task_rust_home="${task_repo_dir}/build/tools/rustup"
task_cargo_home="${task_repo_dir}/build/tools/cargo"
task_tmp_dir="${task_repo_dir}/build/tmp"
task_rust_version="1.97.1"
task_rustup_dist_server="${RUSTUP_DIST_SERVER:-https://mirrors.ustc.edu.cn/rust-static}"
task_rustup_update_root="${RUSTUP_UPDATE_ROOT:-https://mirrors.ustc.edu.cn/rust-static/rustup}"

cd "${task_repo_dir}"

if [[ ! -x "${task_cargo_home}/bin/rustup" && ! -f "${task_bootstrap}" ]]; then
  case "$(uname -s)-$(uname -m)" in
    Darwin-arm64) task_rustup_target="aarch64-apple-darwin" ;;
    Darwin-x86_64) task_rustup_target="x86_64-apple-darwin" ;;
    Linux-aarch64) task_rustup_target="aarch64-unknown-linux-gnu" ;;
    Linux-x86_64) task_rustup_target="x86_64-unknown-linux-gnu" ;;
    *)
      echo "Unsupported rustup bootstrap platform: $(uname -s)-$(uname -m)" >&2
      exit 1
      ;;
  esac
  mkdir -p "$(dirname "${task_bootstrap}")" "${task_tmp_dir}"
  curl --fail --location --proto '=https' --tlsv1.2 \
    --output "${task_bootstrap}" \
    "${task_rustup_dist_server}/rustup/dist/${task_rustup_target}/rustup-init"
fi

mkdir -p "${task_tmp_dir}"
export RUSTUP_HOME="${task_rust_home}"
export CARGO_HOME="${task_cargo_home}"
export RUSTUP_DIST_SERVER="${task_rustup_dist_server}"
export RUSTUP_UPDATE_ROOT="${task_rustup_update_root}"
export TMPDIR="${task_tmp_dir}"
export PATH="${task_cargo_home}/bin:${PATH}"

echo "Rustup mirror: ${RUSTUP_DIST_SERVER}"
echo "Repository-local Rust home: ${RUSTUP_HOME}"
echo "Repository-local Cargo home: ${CARGO_HOME}"

if [[ ! -x "${task_cargo_home}/bin/rustup" ]]; then
  chmod +x "${task_bootstrap}"
  "${task_bootstrap}" \
    --default-toolchain none \
    --profile minimal \
    --no-modify-path \
    -y
fi

task_rustup="${task_cargo_home}/bin/rustup"
task_toolchain=""
for task_candidate in "${task_rust_version}" stable; do
  task_candidate_version="$(
    "${task_rustup}" run "${task_candidate}" rustc --version 2>/dev/null || true
  )"
  if [[ "${task_candidate_version}" == "rustc ${task_rust_version} "* ]]; then
    task_toolchain="${task_candidate}"
    break
  fi
done

if [[ -z "${task_toolchain}" ]]; then
  "${task_rustup}" toolchain install \
    "${task_rust_version}" \
    --profile minimal \
    --component clippy,rustfmt
  task_toolchain="${task_rust_version}"
else
  echo "Reusing repository-local ${task_toolchain} toolchain (${task_rust_version})"
fi

"${task_rustup}" component add \
  --toolchain "${task_toolchain}" \
  clippy rustfmt

"${task_rustup}" run "${task_toolchain}" rustc --version
"${task_rustup}" run "${task_toolchain}" cargo --version
