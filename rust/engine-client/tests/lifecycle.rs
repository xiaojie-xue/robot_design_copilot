use robot_engine_client::{ClientError, EngineClient, EngineCommand, EngineSupervisor};
use serde_json::json;
use std::ffi::OsString;
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicU64, Ordering};
use std::thread;
use std::time::{Duration, Instant};

static NEXT_MARKER: AtomicU64 = AtomicU64::new(1);

fn fake_client(mode: &str, marker: &Path) -> EngineClient {
    let python = std::env::var_os("ROBOT_ENGINE_TEST_PYTHON")
        .expect("ROBOT_ENGINE_TEST_PYTHON must point to the test interpreter");
    let script = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fake_engine.py");
    let args = [
        script.into_os_string(),
        OsString::from(mode),
        marker.as_os_str().to_owned(),
    ];
    EngineClient::spawn(python, args).expect("spawn fake engine")
}

fn marker(name: &str) -> PathBuf {
    let root = PathBuf::from(
        std::env::var_os("ROBOT_ENGINE_TEST_STATE_DIR")
            .expect("ROBOT_ENGINE_TEST_STATE_DIR must stay inside the repository build directory"),
    );
    fs::create_dir_all(&root).expect("create test state directory");
    let sequence = NEXT_MARKER.fetch_add(1, Ordering::Relaxed);
    root.join(format!("{name}-{}-{sequence}", std::process::id()))
}

fn wait_for(path: &Path) {
    let deadline = Instant::now() + Duration::from_secs(3);
    while !path.exists() {
        assert!(
            Instant::now() < deadline,
            "marker did not appear: {}",
            path.display()
        );
        thread::sleep(Duration::from_millis(10));
    }
}

#[test]
fn timeout_sends_matching_cancel() {
    let marker = marker("cancel");
    let client = fake_client("cancel-after-timeout", &marker);
    let error = client
        .request("engine.slow", json!({}), Duration::from_millis(50))
        .expect_err("fake request must time out");
    let request_id = match error {
        ClientError::Timeout { request_id } => request_id,
        other => panic!("unexpected error: {other}"),
    };
    wait_for(&marker);
    assert_eq!(
        fs::read_to_string(marker).expect("read cancel marker"),
        request_id
    );
}

#[test]
fn abrupt_child_exit_is_isolated() {
    let marker = marker("crash-unused");
    let client = fake_client("crash-after-frame", &marker);
    let error = client
        .request("engine.crash", json!({}), Duration::from_secs(2))
        .expect_err("crashed child cannot return a response");
    assert!(matches!(
        error,
        ClientError::ProcessExited { code: Some(42) }
            | ClientError::ProcessExited { code: None }
            | ClientError::Io(_)
    ));
    let deadline = Instant::now() + Duration::from_secs(2);
    while client.is_running().expect("query child state") {
        assert!(Instant::now() < deadline, "crashed child remained running");
        thread::sleep(Duration::from_millis(10));
    }
}

#[test]
fn drop_closes_stdin_and_leaves_no_child() {
    let marker = marker("drop");
    let started = marker.with_extension("started");
    let stopped = marker.with_extension("stopped");
    {
        let client = fake_client("wait-eof", &marker);
        wait_for(&started);
        assert!(client.is_running().expect("query live child"));
    }
    wait_for(&stopped);
}

#[test]
fn explicit_shutdown_is_graceful() {
    let marker = marker("shutdown");
    let started = marker.with_extension("started");
    let stopped = marker.with_extension("stopped");
    let client = fake_client("wait-eof", &marker);
    wait_for(&started);
    let result = client
        .shutdown(Duration::from_secs(2))
        .expect("shutdown child");
    assert!(result.status.success());
    assert!(!result.forced);
    wait_for(&stopped);
}

#[test]
fn progress_is_routed_without_completing_the_request() {
    let marker = marker("progress-unused");
    let client = fake_client("progress-response", &marker);
    let mut progress_events = Vec::new();
    let response = client
        .request_with_progress(
            "engine.progress",
            json!({}),
            Duration::from_secs(2),
            |progress| progress_events.push(progress.clone()),
        )
        .expect("progress fixture response");
    assert_eq!(progress_events.len(), 1);
    assert_eq!(progress_events[0]["progress"]["stage"], "fixture");
    assert_eq!(response["result"]["done"], true);
}

#[test]
fn supervisor_restarts_after_an_isolated_crash() {
    let marker = marker("restart");
    let python = std::env::var_os("ROBOT_ENGINE_TEST_PYTHON")
        .expect("ROBOT_ENGINE_TEST_PYTHON must point to the test interpreter");
    let script = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fake_engine.py");
    let command = EngineCommand::new(PathBuf::from(python)).args([
        script.into_os_string(),
        OsString::from("crash-once"),
        marker.as_os_str().to_owned(),
    ]);
    let supervisor = EngineSupervisor::new(command, Duration::from_secs(2));

    let first = supervisor.start().expect("start first engine");
    assert!(matches!(
        first.request("engine.restart", json!({}), Duration::from_secs(2)),
        Err(ClientError::ProcessExited { .. }) | Err(ClientError::Io(_))
    ));
    wait_for(&marker);

    let second = supervisor.restart().expect("restart engine");
    let response = second
        .request("engine.restart", json!({}), Duration::from_secs(2))
        .expect("restarted engine response");
    assert_eq!(response["result"]["restarted"], true);
    let shutdown = supervisor.shutdown().expect("stop restarted engine");
    assert!(shutdown.expect("running engine").status.success());
}
