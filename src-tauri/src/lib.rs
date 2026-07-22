mod commands;
mod engine;

use engine::DesktopEngine;
use tauri::{Manager, RunEvent};

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    let app = tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .setup(|app| {
            app.manage(DesktopEngine::new(app.handle().clone()));
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            commands::engine_health,
            commands::forward_kinematics,
            commands::restart_engine,
        ])
        .build(tauri::generate_context!())
        .expect("failed to build Robot Design Copilot");

    app.run(|app_handle, event| {
        if matches!(event, RunEvent::Exit | RunEvent::ExitRequested { .. }) {
            app_handle.state::<DesktopEngine>().shutdown();
        }
    });
}
