# Bundled engine staging directory

Tauri external binaries are generated here by `scripts/stage-sidecar.py` and
must use the name `robot-engine-cli-$TARGET_TRIPLE` (plus `.exe` on Windows).
Generated binaries are ignored by Git; the C++ source and lockfiles remain the
source of truth.
