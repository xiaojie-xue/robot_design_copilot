use robot_engine_client::{ClientError, EngineClient};
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
use std::collections::BTreeMap;
use std::process::Command;
use std::sync::{Arc, Mutex, MutexGuard};
use std::time::Duration;
use tauri::AppHandle;
use tauri_plugin_shell::ShellExt;

const REQUEST_TIMEOUT: Duration = Duration::from_secs(3);
const SHUTDOWN_TIMEOUT: Duration = Duration::from_secs(2);
const SIDECAR_NAME: &str = "robot-engine-cli";

pub struct DesktopEngine {
    app: AppHandle,
    client: Mutex<Option<Arc<EngineClient>>>,
}

impl DesktopEngine {
    pub fn new(app: AppHandle) -> Self {
        Self {
            app,
            client: Mutex::new(None),
        }
    }

    pub fn health(&self) -> Result<EngineHealth, CommandError> {
        let response = self
            .client()?
            .request("engine.health", json!({}), REQUEST_TIMEOUT)
            .map_err(CommandError::from)?;
        parse_health(response)
    }

    pub fn forward(
        &self,
        request: ForwardKinematicsRequest,
    ) -> Result<ForwardKinematicsResult, CommandError> {
        if request
            .joint_positions_rad
            .iter()
            .any(|value| !value.is_finite())
        {
            return Err(CommandError::new(
                "invalid_joint_position",
                "Every joint position must be finite.",
                false,
            ));
        }
        let response = self
            .client()?
            .request(
                "kinematics.forward",
                json!({"joint_positions_rad": request.joint_positions_rad}),
                REQUEST_TIMEOUT,
            )
            .map_err(CommandError::from)?;
        parse_forward(response)
    }

    pub fn restart(&self) -> Result<EngineHealth, CommandError> {
        let previous = lock(&self.client).take();
        if let Some(client) = previous {
            client
                .shutdown(SHUTDOWN_TIMEOUT)
                .map_err(CommandError::from)?;
        }
        *lock(&self.client) = Some(Arc::new(self.spawn()?));
        self.health()
    }

    pub fn shutdown(&self) {
        if let Some(client) = lock(&self.client).take() {
            let _ = client.shutdown(SHUTDOWN_TIMEOUT);
        }
    }

    fn client(&self) -> Result<Arc<EngineClient>, CommandError> {
        let mut slot = lock(&self.client);
        if let Some(client) = slot.as_ref() {
            if client.is_running().map_err(CommandError::from)? {
                return Ok(Arc::clone(client));
            }
        }
        let client = Arc::new(self.spawn()?);
        *slot = Some(Arc::clone(&client));
        Ok(client)
    }

    fn spawn(&self) -> Result<EngineClient, CommandError> {
        let command = self
            .app
            .shell()
            .sidecar(SIDECAR_NAME)
            .map_err(|error| CommandError::new("engine_start_failed", error.to_string(), true))?
            .set_raw_out(true);
        let command: Command = command.into();
        EngineClient::spawn_command(command).map_err(CommandError::from)
    }
}

impl Drop for DesktopEngine {
    fn drop(&mut self) {
        self.shutdown();
    }
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ForwardKinematicsRequest {
    joint_positions_rad: [f64; 7],
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct EngineHealth {
    engine_version: String,
    protocol_version: u64,
    status: String,
    capabilities: Vec<String>,
    dependencies: BTreeMap<String, String>,
}

#[derive(Debug, Deserialize, Serialize)]
struct ForwardResultPayload {
    model_id: String,
    base_frame: String,
    tool_frame: String,
    joint_count: usize,
    translation_m: [f64; 3],
    orientation_xyzw: [f64; 4],
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ForwardKinematicsResult {
    engine_version: String,
    model_id: String,
    base_frame: String,
    tool_frame: String,
    joint_count: usize,
    translation_m: [f64; 3],
    orientation_xyzw: [f64; 4],
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct CommandError {
    code: String,
    message: String,
    retryable: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    details: Option<Value>,
}

impl CommandError {
    fn new(code: impl Into<String>, message: impl Into<String>, retryable: bool) -> Self {
        Self {
            code: code.into(),
            message: message.into(),
            retryable,
            details: None,
        }
    }
}

impl From<ClientError> for CommandError {
    fn from(error: ClientError) -> Self {
        match error {
            ClientError::Engine {
                code,
                message,
                retryable,
                details,
                ..
            } => Self {
                code,
                message,
                retryable,
                details,
            },
            ClientError::Timeout { request_id } => Self {
                code: "engine_timeout".to_owned(),
                message: format!("Engine request {request_id} timed out."),
                retryable: true,
                details: Some(json!({"request_id": request_id})),
            },
            ClientError::ProcessExited { code } => Self {
                code: "engine_exited".to_owned(),
                message: "The engineering engine exited unexpectedly.".to_owned(),
                retryable: true,
                details: Some(json!({"exit_code": code})),
            },
            ClientError::Closed => Self::new(
                "engine_closed",
                "The engineering engine is not running.",
                true,
            ),
            ClientError::Io(message) => Self::new("engine_io", message, true),
            ClientError::Protocol(message) => Self::new("engine_protocol", message, false),
        }
    }
}

fn parse_health(response: Value) -> Result<EngineHealth, CommandError> {
    let engine_version = required_string(&response, "engine_version")?;
    let result = response.get("result").ok_or_else(|| {
        CommandError::new("engine_protocol", "Health response has no result.", false)
    })?;
    let protocol_version = result
        .get("protocol_version")
        .and_then(Value::as_u64)
        .ok_or_else(|| {
            CommandError::new(
                "engine_protocol",
                "Health response has no protocol version.",
                false,
            )
        })?;
    let status = required_string(result, "status")?;
    let capabilities = serde_json::from_value(
        result
            .get("capabilities")
            .cloned()
            .ok_or_else(|| CommandError::new("engine_protocol", "Missing capabilities.", false))?,
    )
    .map_err(protocol_decode_error)?;
    let dependencies = serde_json::from_value(
        result
            .get("dependencies")
            .cloned()
            .ok_or_else(|| CommandError::new("engine_protocol", "Missing dependencies.", false))?,
    )
    .map_err(protocol_decode_error)?;
    Ok(EngineHealth {
        engine_version,
        protocol_version,
        status,
        capabilities,
        dependencies,
    })
}

fn parse_forward(response: Value) -> Result<ForwardKinematicsResult, CommandError> {
    let engine_version = required_string(&response, "engine_version")?;
    let payload: ForwardResultPayload = serde_json::from_value(
        response
            .get("result")
            .cloned()
            .ok_or_else(|| CommandError::new("engine_protocol", "Missing FK result.", false))?,
    )
    .map_err(protocol_decode_error)?;
    Ok(ForwardKinematicsResult {
        engine_version,
        model_id: payload.model_id,
        base_frame: payload.base_frame,
        tool_frame: payload.tool_frame,
        joint_count: payload.joint_count,
        translation_m: payload.translation_m,
        orientation_xyzw: payload.orientation_xyzw,
    })
}

fn required_string(value: &Value, field: &str) -> Result<String, CommandError> {
    value
        .get(field)
        .and_then(Value::as_str)
        .filter(|text| !text.is_empty())
        .map(ToOwned::to_owned)
        .ok_or_else(|| {
            CommandError::new(
                "engine_protocol",
                format!("Engine response has invalid {field}."),
                false,
            )
        })
}

fn protocol_decode_error(error: serde_json::Error) -> CommandError {
    CommandError::new("engine_protocol", error.to_string(), false)
}

fn lock<T>(mutex: &Mutex<T>) -> MutexGuard<'_, T> {
    mutex
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_versioned_forward_result() {
        let parsed = parse_forward(json!({
            "engine_version": "0.1.0",
            "result": {
                "model_id": "reference_arm_7dof_v1",
                "base_frame": "base",
                "tool_frame": "tool0",
                "joint_count": 7,
                "translation_m": [1.26, 0.0, 0.54],
                "orientation_xyzw": [0.0, 0.0, 0.0, 1.0]
            }
        }))
        .expect("parse forward result");
        assert_eq!(parsed.translation_m, [1.26, 0.0, 0.54]);
        assert_eq!(parsed.orientation_xyzw, [0.0, 0.0, 0.0, 1.0]);
    }

    #[test]
    fn rejects_forward_result_with_wrong_vector_shape() {
        let error = parse_forward(json!({
            "engine_version": "0.1.0",
            "result": {
                "model_id": "reference_arm_7dof_v1",
                "base_frame": "base",
                "tool_frame": "tool0",
                "joint_count": 7,
                "translation_m": [1.0, 2.0],
                "orientation_xyzw": [0.0, 0.0, 0.0, 1.0]
            }
        }))
        .expect_err("reject malformed translation");
        assert_eq!(error.code, "engine_protocol");
    }
}
