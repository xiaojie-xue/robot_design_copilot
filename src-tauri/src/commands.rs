use crate::engine::{
    CommandError, DesktopEngine, EngineHealth, ForwardKinematicsRequest, ForwardKinematicsResult,
};
use tauri::State;

#[tauri::command(async)]
pub fn engine_health(engine: State<'_, DesktopEngine>) -> Result<EngineHealth, CommandError> {
    engine.health()
}

#[tauri::command(async)]
pub fn forward_kinematics(
    engine: State<'_, DesktopEngine>,
    request: ForwardKinematicsRequest,
) -> Result<ForwardKinematicsResult, CommandError> {
    engine.forward(request)
}

#[tauri::command(async)]
pub fn restart_engine(engine: State<'_, DesktopEngine>) -> Result<EngineHealth, CommandError> {
    engine.restart()
}
