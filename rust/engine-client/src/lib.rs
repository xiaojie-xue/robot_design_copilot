use serde_json::{Map, Value, json};
use std::collections::HashMap;
use std::ffi::{OsStr, OsString};
use std::fmt;
use std::io::{self, Read, Write};
use std::path::{Path, PathBuf};
use std::process::{Child, ChildStdin, Command, ExitStatus, Stdio};
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex, MutexGuard, mpsc};
use std::thread;
use std::time::{Duration, Instant};

pub const PROTOCOL_VERSION: u64 = 1;
pub const MAX_FRAME_SIZE: usize = 16 * 1024 * 1024;
const MAX_CAPTURED_STDERR: usize = 64 * 1024;

#[derive(Clone, Debug, PartialEq)]
pub enum ClientError {
    Closed,
    ProcessExited {
        code: Option<i32>,
    },
    Io(String),
    Protocol(String),
    Timeout {
        request_id: String,
    },
    Engine {
        request_id: String,
        code: String,
        message: String,
        retryable: bool,
        details: Option<Value>,
    },
}

impl fmt::Display for ClientError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Closed => write!(formatter, "the engine process is closed"),
            Self::ProcessExited { code } => {
                write!(formatter, "the engine process exited with code {code:?}")
            }
            Self::Io(message) => write!(formatter, "engine I/O failed: {message}"),
            Self::Protocol(message) => write!(formatter, "engine protocol error: {message}"),
            Self::Timeout { request_id } => {
                write!(formatter, "engine request {request_id} timed out")
            }
            Self::Engine { code, message, .. } => {
                write!(formatter, "engine returned {code}: {message}")
            }
        }
    }
}

impl std::error::Error for ClientError {}

#[derive(Debug)]
pub struct ShutdownResult {
    pub status: ExitStatus,
    pub forced: bool,
}

enum EngineEvent {
    Progress(Value),
    Response(Value),
    Error(ClientError),
}

type PendingRequests = Arc<Mutex<HashMap<String, mpsc::Sender<EngineEvent>>>>;

#[derive(Clone, Debug)]
pub struct EngineCommand {
    program: PathBuf,
    args: Vec<OsString>,
}

impl EngineCommand {
    pub fn new(program: impl Into<PathBuf>) -> Self {
        Self {
            program: program.into(),
            args: Vec::new(),
        }
    }

    pub fn arg(mut self, arg: impl Into<OsString>) -> Self {
        self.args.push(arg.into());
        self
    }

    pub fn args<I, S>(mut self, args: I) -> Self
    where
        I: IntoIterator<Item = S>,
        S: Into<OsString>,
    {
        self.args.extend(args.into_iter().map(Into::into));
        self
    }

    pub fn spawn(&self) -> Result<EngineClient, ClientError> {
        EngineClient::spawn(&self.program, &self.args)
    }
}

pub struct EngineSupervisor {
    command: EngineCommand,
    client: Mutex<Option<Arc<EngineClient>>>,
    shutdown_timeout: Duration,
}

impl EngineSupervisor {
    pub fn new(command: EngineCommand, shutdown_timeout: Duration) -> Self {
        Self {
            command,
            client: Mutex::new(None),
            shutdown_timeout,
        }
    }

    pub fn start(&self) -> Result<Arc<EngineClient>, ClientError> {
        let mut slot = lock(&self.client);
        if let Some(client) = slot.as_ref()
            && client.is_running()?
        {
            return Ok(Arc::clone(client));
        }
        let client = Arc::new(self.command.spawn()?);
        *slot = Some(Arc::clone(&client));
        Ok(client)
    }

    pub fn restart(&self) -> Result<Arc<EngineClient>, ClientError> {
        let previous = lock(&self.client).take();
        if let Some(client) = previous {
            let _ = client.shutdown(self.shutdown_timeout);
        }
        let client = Arc::new(self.command.spawn()?);
        *lock(&self.client) = Some(Arc::clone(&client));
        Ok(client)
    }

    pub fn shutdown(&self) -> Result<Option<ShutdownResult>, ClientError> {
        let client = lock(&self.client).take();
        client
            .map(|client| client.shutdown(self.shutdown_timeout))
            .transpose()
    }
}

impl Drop for EngineSupervisor {
    fn drop(&mut self) {
        if let Some(client) = lock(&self.client).take() {
            let _ = client.shutdown(self.shutdown_timeout);
        }
    }
}

pub struct EngineClient {
    child: Arc<Mutex<Option<Child>>>,
    stdin: Mutex<Option<ChildStdin>>,
    pending: PendingRequests,
    terminal_error: Arc<Mutex<Option<ClientError>>>,
    stderr: Arc<Mutex<Vec<u8>>>,
    next_request_id: AtomicU64,
    closed: AtomicBool,
}

impl EngineClient {
    pub fn spawn<P, I, S>(program: P, args: I) -> Result<Self, ClientError>
    where
        P: AsRef<Path>,
        I: IntoIterator<Item = S>,
        S: AsRef<OsStr>,
    {
        let mut command = Command::new(program.as_ref());
        command.args(args);
        Self::spawn_command(command)
    }

    pub fn spawn_command(mut command: Command) -> Result<Self, ClientError> {
        let mut child = command
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
            .map_err(io_error)?;

        let stdin = child
            .stdin
            .take()
            .ok_or_else(|| ClientError::Io("the engine did not expose a stdin pipe".to_owned()))?;
        let stdout = child
            .stdout
            .take()
            .ok_or_else(|| ClientError::Io("the engine did not expose a stdout pipe".to_owned()))?;
        let stderr_pipe = child
            .stderr
            .take()
            .ok_or_else(|| ClientError::Io("the engine did not expose a stderr pipe".to_owned()))?;

        let child = Arc::new(Mutex::new(Some(child)));
        let pending = PendingRequests::default();
        let terminal_error = Arc::new(Mutex::new(None));
        let stderr = Arc::new(Mutex::new(Vec::new()));

        spawn_stdout_reader(
            stdout,
            Arc::clone(&pending),
            Arc::clone(&terminal_error),
            Arc::clone(&child),
        );
        spawn_stderr_reader(stderr_pipe, Arc::clone(&stderr));

        Ok(Self {
            child,
            stdin: Mutex::new(Some(stdin)),
            pending,
            terminal_error,
            stderr,
            next_request_id: AtomicU64::new(1),
            closed: AtomicBool::new(false),
        })
    }

    pub fn process_id(&self) -> Option<u32> {
        lock(&self.child).as_ref().map(Child::id)
    }

    pub fn is_running(&self) -> Result<bool, ClientError> {
        let mut child = lock(&self.child);
        let Some(child) = child.as_mut() else {
            return Ok(false);
        };
        child
            .try_wait()
            .map(|status| status.is_none())
            .map_err(io_error)
    }

    pub fn stderr_snapshot(&self) -> Vec<u8> {
        lock(&self.stderr).clone()
    }

    pub fn request(
        &self,
        method: &str,
        params: Value,
        timeout: Duration,
    ) -> Result<Value, ClientError> {
        self.request_with_progress(method, params, timeout, |_| {})
    }

    pub fn request_with_progress<F>(
        &self,
        method: &str,
        params: Value,
        timeout: Duration,
        mut on_progress: F,
    ) -> Result<Value, ClientError>
    where
        F: FnMut(&Value),
    {
        validate_method(method)?;
        if !params.is_object() {
            return Err(ClientError::Protocol(
                "request params must be a JSON object".to_owned(),
            ));
        }
        if self.closed.load(Ordering::Acquire) {
            return Err(ClientError::Closed);
        }
        if let Some(error) = lock(&self.terminal_error).clone() {
            return Err(error);
        }

        let request_number = self.next_request_id.fetch_add(1, Ordering::Relaxed);
        let request_id = format!("rust-{request_number}");
        let envelope = json!({
            "protocol_version": PROTOCOL_VERSION,
            "type": "request",
            "request_id": request_id,
            "method": method,
            "params": params,
        });
        let (sender, receiver) = mpsc::channel();
        lock(&self.pending).insert(request_id.clone(), sender);

        if let Err(error) = self.write_envelope(&envelope) {
            lock(&self.pending).remove(&request_id);
            return Err(error);
        }

        let deadline = Instant::now() + timeout;
        loop {
            let remaining = deadline.saturating_duration_since(Instant::now());
            match receiver.recv_timeout(remaining) {
                Ok(EngineEvent::Progress(progress)) => on_progress(&progress),
                Ok(EngineEvent::Response(response)) => return Ok(response),
                Ok(EngineEvent::Error(error)) => return Err(error),
                Err(mpsc::RecvTimeoutError::Timeout) => {
                    lock(&self.pending).remove(&request_id);
                    let _ = self.cancel(&request_id);
                    return Err(ClientError::Timeout { request_id });
                }
                Err(mpsc::RecvTimeoutError::Disconnected) => {
                    return Err(lock(&self.terminal_error)
                        .clone()
                        .unwrap_or(ClientError::Closed));
                }
            }
        }
    }

    pub fn cancel(&self, request_id: &str) -> Result<(), ClientError> {
        validate_request_id(request_id)?;
        let envelope = json!({
            "protocol_version": PROTOCOL_VERSION,
            "type": "cancel",
            "request_id": request_id,
        });
        self.write_envelope(&envelope)
    }

    pub fn shutdown(&self, timeout: Duration) -> Result<ShutdownResult, ClientError> {
        if self.closed.swap(true, Ordering::AcqRel) {
            let mut child = lock(&self.child);
            let child = child.as_mut().ok_or(ClientError::Closed)?;
            let status = child
                .try_wait()
                .map_err(io_error)?
                .ok_or(ClientError::Closed)?;
            return Ok(ShutdownResult {
                status,
                forced: false,
            });
        }

        lock(&self.stdin).take();
        fail_all(&self.pending, ClientError::Closed);

        let deadline = Instant::now() + timeout;
        loop {
            {
                let mut child = lock(&self.child);
                let child = child.as_mut().ok_or(ClientError::Closed)?;
                if let Some(status) = child.try_wait().map_err(io_error)? {
                    return Ok(ShutdownResult {
                        status,
                        forced: false,
                    });
                }
                if Instant::now() >= deadline {
                    child.kill().map_err(io_error)?;
                    let status = child.wait().map_err(io_error)?;
                    return Ok(ShutdownResult {
                        status,
                        forced: true,
                    });
                }
            }
            thread::sleep(Duration::from_millis(10));
        }
    }

    fn write_envelope(&self, envelope: &Value) -> Result<(), ClientError> {
        let payload = serde_json::to_vec(envelope)
            .map_err(|error| ClientError::Protocol(error.to_string()))?;
        if payload.is_empty() || payload.len() > MAX_FRAME_SIZE {
            return Err(ClientError::Protocol(
                "outgoing frame size is outside protocol bounds".to_owned(),
            ));
        }
        let length = u32::try_from(payload.len()).map_err(|_| {
            ClientError::Protocol("outgoing frame does not fit the wire length".to_owned())
        })?;
        let mut stdin = lock(&self.stdin);
        let writer = stdin.as_mut().ok_or(ClientError::Closed)?;
        writer.write_all(&length.to_be_bytes()).map_err(io_error)?;
        writer.write_all(&payload).map_err(io_error)?;
        writer.flush().map_err(io_error)
    }
}

impl Drop for EngineClient {
    fn drop(&mut self) {
        if !self.closed.load(Ordering::Acquire) {
            let _ = self.shutdown(Duration::from_millis(500));
        }
    }
}

fn spawn_stdout_reader<R>(
    mut reader: R,
    pending: PendingRequests,
    terminal_error: Arc<Mutex<Option<ClientError>>>,
    child: Arc<Mutex<Option<Child>>>,
) where
    R: Read + Send + 'static,
{
    thread::spawn(move || {
        loop {
            match read_frame(&mut reader).and_then(parse_event) {
                Ok(Some((request_id, event, completes_request))) => {
                    let sender = if completes_request {
                        lock(&pending).remove(&request_id)
                    } else {
                        lock(&pending).get(&request_id).cloned()
                    };
                    if let Some(sender) = sender {
                        let _ = sender.send(event);
                    }
                }
                Ok(None) => {
                    let code = lock(&child)
                        .as_mut()
                        .and_then(|process| process.try_wait().ok().flatten())
                        .and_then(|status| status.code());
                    let error = ClientError::ProcessExited { code };
                    *lock(&terminal_error) = Some(error.clone());
                    fail_all(&pending, error);
                    return;
                }
                Err(error) => {
                    *lock(&terminal_error) = Some(error.clone());
                    fail_all(&pending, error);
                    return;
                }
            }
        }
    });
}

fn spawn_stderr_reader<R>(mut reader: R, captured: Arc<Mutex<Vec<u8>>>)
where
    R: Read + Send + 'static,
{
    thread::spawn(move || {
        let mut buffer = [0_u8; 4096];
        while let Ok(count) = reader.read(&mut buffer) {
            if count == 0 {
                break;
            }
            let mut output = lock(&captured);
            let remaining = MAX_CAPTURED_STDERR.saturating_sub(output.len());
            output.extend_from_slice(&buffer[..count.min(remaining)]);
        }
    });
}

fn read_frame<R: Read>(reader: &mut R) -> Result<Option<Vec<u8>>, ClientError> {
    let mut header = [0_u8; 4];
    let mut header_bytes = 0;
    while header_bytes < header.len() {
        match reader.read(&mut header[header_bytes..]) {
            Ok(0) if header_bytes == 0 => return Ok(None),
            Ok(0) => {
                return Err(ClientError::Protocol(
                    "engine closed stdout during a frame header".to_owned(),
                ));
            }
            Ok(count) => header_bytes += count,
            Err(error) if error.kind() == io::ErrorKind::Interrupted => {}
            Err(error) => return Err(io_error(error)),
        }
    }

    let length = u32::from_be_bytes(header) as usize;
    if length == 0 || length > MAX_FRAME_SIZE {
        return Err(ClientError::Protocol(format!(
            "engine sent invalid frame length {length}"
        )));
    }
    let mut payload = vec![0_u8; length];
    reader.read_exact(&mut payload).map_err(|error| {
        if error.kind() == io::ErrorKind::UnexpectedEof {
            ClientError::Protocol("engine closed stdout during a frame payload".to_owned())
        } else {
            io_error(error)
        }
    })?;
    Ok(Some(payload))
}

fn parse_event(
    payload: Option<Vec<u8>>,
) -> Result<Option<(String, EngineEvent, bool)>, ClientError> {
    let Some(payload) = payload else {
        return Ok(None);
    };
    let envelope: Value = serde_json::from_slice(&payload)
        .map_err(|error| ClientError::Protocol(format!("invalid JSON response: {error}")))?;
    let envelope = envelope
        .as_object()
        .ok_or_else(|| ClientError::Protocol("engine envelope is not an object".to_owned()))?;
    if envelope.get("protocol_version").and_then(Value::as_u64) != Some(PROTOCOL_VERSION) {
        return Err(ClientError::Protocol(
            "engine response uses an unsupported protocol version".to_owned(),
        ));
    }
    let request_id = envelope
        .get("request_id")
        .and_then(Value::as_str)
        .ok_or_else(|| ClientError::Protocol("engine response has no request_id".to_owned()))?
        .to_owned();
    validate_request_id(&request_id)?;
    match envelope.get("type").and_then(Value::as_str) {
        Some("progress") => {
            validate_fields(
                envelope,
                &["protocol_version", "type", "request_id", "progress"],
                &["protocol_version", "type", "request_id", "progress"],
                "progress envelope",
            )?;
            validate_progress(envelope.get("progress"))?;
            Ok(Some((
                request_id,
                EngineEvent::Progress(Value::Object(envelope.clone())),
                false,
            )))
        }
        Some("response") => {
            validate_fields(
                envelope,
                &[
                    "protocol_version",
                    "type",
                    "request_id",
                    "engine_version",
                    "result",
                ],
                &[
                    "protocol_version",
                    "type",
                    "request_id",
                    "engine_version",
                    "result",
                ],
                "response envelope",
            )?;
            required_nonempty_string(envelope.get("engine_version"), "engine_version")?;
            Ok(Some((
                request_id,
                EngineEvent::Response(Value::Object(envelope.clone())),
                true,
            )))
        }
        Some("error") => {
            validate_fields(
                envelope,
                &[
                    "protocol_version",
                    "type",
                    "request_id",
                    "engine_version",
                    "error",
                ],
                &[
                    "protocol_version",
                    "type",
                    "request_id",
                    "engine_version",
                    "error",
                ],
                "error envelope",
            )?;
            required_nonempty_string(envelope.get("engine_version"), "engine_version")?;
            let error = envelope
                .get("error")
                .and_then(Value::as_object)
                .ok_or_else(|| ClientError::Protocol("malformed engine error".to_owned()))?;
            validate_fields(
                error,
                &["code", "message", "retryable"],
                &["code", "message", "retryable", "details"],
                "error detail",
            )?;
            let code = required_nonempty_string(error.get("code"), "error.code")?;
            let message = required_nonempty_string(error.get("message"), "error.message")?;
            let retryable = error
                .get("retryable")
                .and_then(Value::as_bool)
                .ok_or_else(|| ClientError::Protocol("invalid error.retryable".to_owned()))?;
            let details = match error.get("details") {
                Some(Value::Object(_)) => error.get("details").cloned(),
                Some(_) => {
                    return Err(ClientError::Protocol("invalid error.details".to_owned()));
                }
                None => None,
            };
            Ok(Some((
                request_id.clone(),
                EngineEvent::Error(ClientError::Engine {
                    request_id,
                    code,
                    message,
                    retryable,
                    details,
                }),
                true,
            )))
        }
        _ => Err(ClientError::Protocol(
            "engine sent an unsupported envelope type".to_owned(),
        )),
    }
}

fn validate_fields(
    object: &Map<String, Value>,
    required: &[&str],
    allowed: &[&str],
    context: &str,
) -> Result<(), ClientError> {
    if let Some(field) = required.iter().find(|field| !object.contains_key(**field)) {
        return Err(ClientError::Protocol(format!(
            "{context} is missing {field}"
        )));
    }
    if let Some(field) = object
        .keys()
        .find(|field| !allowed.contains(&field.as_str()))
    {
        return Err(ClientError::Protocol(format!(
            "{context} contains unsupported field {field}"
        )));
    }
    Ok(())
}

fn validate_progress(value: Option<&Value>) -> Result<(), ClientError> {
    let progress = value
        .and_then(Value::as_object)
        .ok_or_else(|| ClientError::Protocol("invalid progress".to_owned()))?;
    validate_fields(
        progress,
        &["fraction", "stage"],
        &["fraction", "stage", "message"],
        "progress",
    )?;
    progress
        .get("fraction")
        .and_then(Value::as_f64)
        .filter(|fraction| fraction.is_finite() && (0.0..=1.0).contains(fraction))
        .ok_or_else(|| ClientError::Protocol("invalid progress.fraction".to_owned()))?;
    required_nonempty_string(progress.get("stage"), "progress.stage")?;
    if !matches!(progress.get("message"), None | Some(Value::String(_))) {
        return Err(ClientError::Protocol("invalid progress.message".to_owned()));
    }
    Ok(())
}

fn required_nonempty_string(value: Option<&Value>, field: &str) -> Result<String, ClientError> {
    value
        .and_then(Value::as_str)
        .filter(|text| !text.is_empty())
        .map(ToOwned::to_owned)
        .ok_or_else(|| ClientError::Protocol(format!("invalid {field}")))
}

fn validate_request_id(request_id: &str) -> Result<(), ClientError> {
    let mut bytes = request_id.bytes();
    if !matches!(bytes.next(), Some(byte) if byte.is_ascii_alphanumeric())
        || request_id.len() > 128
        || !bytes.all(|byte| byte.is_ascii_alphanumeric() || b"._:-".contains(&byte))
    {
        return Err(ClientError::Protocol(
            "request_id is outside the protocol identifier grammar".to_owned(),
        ));
    }
    Ok(())
}

fn validate_method(method: &str) -> Result<(), ClientError> {
    let mut bytes = method.bytes();
    if !matches!(bytes.next(), Some(b'a'..=b'z'))
        || method.len() > 128
        || !bytes.all(|byte| {
            byte.is_ascii_lowercase() || byte.is_ascii_digit() || b"._-".contains(&byte)
        })
    {
        return Err(ClientError::Protocol(
            "method is outside the protocol identifier grammar".to_owned(),
        ));
    }
    Ok(())
}

fn fail_all(pending: &PendingRequests, error: ClientError) {
    let requests = std::mem::take(&mut *lock(pending));
    for (_, sender) in requests {
        let _ = sender.send(EngineEvent::Error(error.clone()));
    }
}

fn io_error(error: io::Error) -> ClientError {
    ClientError::Io(error.to_string())
}

fn lock<T>(mutex: &Mutex<T>) -> MutexGuard<'_, T> {
    mutex
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;

    fn framed(payload: &[u8]) -> Vec<u8> {
        let mut output = Vec::from((payload.len() as u32).to_be_bytes());
        output.extend_from_slice(payload);
        output
    }

    #[test]
    fn reads_one_bounded_frame() {
        let payload = br#"{"protocol_version":1}"#;
        let mut input = Cursor::new(framed(payload));
        assert_eq!(
            read_frame(&mut input).expect("read frame"),
            Some(payload.to_vec())
        );
        assert_eq!(read_frame(&mut input).expect("read EOF"), None);
    }

    #[test]
    fn rejects_zero_oversized_and_truncated_frames() {
        let mut zero = Cursor::new(0_u32.to_be_bytes());
        assert!(matches!(
            read_frame(&mut zero),
            Err(ClientError::Protocol(_))
        ));

        let mut oversized = Cursor::new(((MAX_FRAME_SIZE as u32) + 1).to_be_bytes());
        assert!(matches!(
            read_frame(&mut oversized),
            Err(ClientError::Protocol(_))
        ));

        let mut truncated = Cursor::new([0, 0, 0, 3, b'{']);
        assert!(matches!(
            read_frame(&mut truncated),
            Err(ClientError::Protocol(_))
        ));
    }

    #[test]
    fn rejects_malformed_json_and_wrong_protocol_version() {
        assert!(matches!(
            parse_event(Some(b"{".to_vec())),
            Err(ClientError::Protocol(_))
        ));
        let wrong_version = serde_json::to_vec(&json!({
            "protocol_version": 2,
            "type": "response",
            "request_id": "rust-1",
            "engine_version": "fixture",
            "result": {}
        }))
        .expect("serialize fixture");
        assert!(matches!(
            parse_event(Some(wrong_version)),
            Err(ClientError::Protocol(_))
        ));
    }

    #[test]
    fn validates_response_envelope_shape() {
        let valid = serde_json::to_vec(&json!({
            "protocol_version": 1,
            "type": "response",
            "request_id": "rust-1",
            "engine_version": "fixture",
            "result": {"status": "ok"}
        }))
        .expect("serialize response");
        assert!(matches!(
            parse_event(Some(valid)),
            Ok(Some((request_id, EngineEvent::Response(_), true)))
                if request_id == "rust-1"
        ));

        for invalid in [
            json!({
                "protocol_version": 1,
                "type": "response",
                "request_id": "rust-1",
                "engine_version": "fixture"
            }),
            json!({
                "protocol_version": 1,
                "type": "response",
                "request_id": "bad request id",
                "engine_version": "fixture",
                "result": {}
            }),
            json!({
                "protocol_version": 1,
                "type": "response",
                "request_id": "rust-1",
                "engine_version": "",
                "result": {}
            }),
            json!({
                "protocol_version": 1,
                "type": "response",
                "request_id": "rust-1",
                "engine_version": "fixture",
                "result": {},
                "future": true
            }),
        ] {
            let payload = serde_json::to_vec(&invalid).expect("serialize invalid response");
            assert!(matches!(
                parse_event(Some(payload)),
                Err(ClientError::Protocol(_))
            ));
        }
    }

    #[test]
    fn validates_progress_envelope_shape() {
        let valid = serde_json::to_vec(&json!({
            "protocol_version": 1,
            "type": "progress",
            "request_id": "rust-2",
            "progress": {
                "fraction": 0.5,
                "stage": "solving",
                "message": "halfway"
            }
        }))
        .expect("serialize progress");
        assert!(matches!(
            parse_event(Some(valid)),
            Ok(Some((request_id, EngineEvent::Progress(_), false)))
                if request_id == "rust-2"
        ));

        for invalid_progress in [
            json!({"fraction": 1.1, "stage": "solving"}),
            json!({"fraction": 0.5, "stage": ""}),
            json!({"fraction": 0.5, "stage": "solving", "message": 3}),
            json!({"fraction": 0.5, "stage": "solving", "future": true}),
        ] {
            let payload = serde_json::to_vec(&json!({
                "protocol_version": 1,
                "type": "progress",
                "request_id": "rust-2",
                "progress": invalid_progress
            }))
            .expect("serialize invalid progress");
            assert!(matches!(
                parse_event(Some(payload)),
                Err(ClientError::Protocol(_))
            ));
        }
    }

    #[test]
    fn validates_structured_error_shape() {
        let valid = serde_json::to_vec(&json!({
            "protocol_version": 1,
            "type": "error",
            "request_id": "rust-3",
            "engine_version": "fixture",
            "error": {
                "code": "invalid_params",
                "message": "bad input",
                "retryable": false,
                "details": {"field": "joint_positions_rad"}
            }
        }))
        .expect("serialize error");
        assert!(matches!(
            parse_event(Some(valid)),
            Ok(Some((request_id, EngineEvent::Error(ClientError::Engine { code, .. }), true)))
                if request_id == "rust-3" && code == "invalid_params"
        ));

        for invalid_error in [
            json!({"code": "", "message": "bad input", "retryable": false}),
            json!({"code": "invalid_params", "message": "", "retryable": false}),
            json!({"code": "invalid_params", "message": "bad input", "retryable": 0}),
            json!({
                "code": "invalid_params",
                "message": "bad input",
                "retryable": false,
                "details": []
            }),
            json!({
                "code": "invalid_params",
                "message": "bad input",
                "retryable": false,
                "future": true
            }),
        ] {
            let payload = serde_json::to_vec(&json!({
                "protocol_version": 1,
                "type": "error",
                "request_id": "rust-3",
                "engine_version": "fixture",
                "error": invalid_error
            }))
            .expect("serialize invalid error");
            assert!(matches!(
                parse_event(Some(payload)),
                Err(ClientError::Protocol(_))
            ));
        }
    }

    #[test]
    fn validates_method_contract() {
        assert!(validate_method("engine.health").is_ok());
        assert!(validate_method("Engine.Health").is_err());
        assert!(validate_method("").is_err());
    }

    #[test]
    fn validates_request_id_contract() {
        assert!(validate_request_id("rust-1:child_2").is_ok());
        assert!(validate_request_id("").is_err());
        assert!(validate_request_id("has space").is_err());
        assert!(validate_request_id(&"x".repeat(129)).is_err());
    }
}
