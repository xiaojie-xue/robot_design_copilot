use robot_engine_client::{ClientError, EngineClient};
use serde_json::json;
use std::path::PathBuf;
use std::sync::Arc;
use std::thread;
use std::time::Duration;

fn real_client() -> EngineClient {
    let binary = PathBuf::from(
        std::env::var_os("ROBOT_ENGINE_TEST_BINARY")
            .expect("ROBOT_ENGINE_TEST_BINARY must point to robot-engine-cli"),
    );
    EngineClient::spawn(binary, std::iter::empty::<&str>()).expect("spawn real engine")
}

#[test]
fn health_and_forward_kinematics_cross_the_rust_cpp_boundary() {
    let client = real_client();
    let health = client
        .request("engine.health", json!({}), Duration::from_secs(3))
        .expect("health response");
    assert_eq!(health["type"], "response");
    assert_eq!(health["result"]["status"], "ok");
    assert_eq!(
        health["result"]["capabilities"],
        json!(["engine.health", "design.calculate", "kinematics.forward"])
    );

    let forward = client
        .request(
            "kinematics.forward",
            json!({"joint_positions_rad": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]}),
            Duration::from_secs(3),
        )
        .expect("forward response");
    assert_eq!(forward["result"]["model_id"], "reference_arm_7dof");
    assert_eq!(forward["result"]["base_frame"], "base");
    assert_eq!(forward["result"]["tool_frame"], "tool0");
    assert_eq!(forward["result"]["translation_m"], json!([1.26, 0.0, 0.54]));

    let shutdown = client
        .shutdown(Duration::from_secs(2))
        .expect("graceful real-engine shutdown");
    assert!(shutdown.status.success());
    assert!(!shutdown.forced);
    assert!(client.stderr_snapshot().is_empty());
}

#[test]
fn concurrent_requests_are_matched_by_request_id() {
    let client = Arc::new(real_client());
    let workers: Vec<_> = (0..4)
        .map(|_| {
            let client = Arc::clone(&client);
            thread::spawn(move || {
                client
                    .request("engine.health", json!({}), Duration::from_secs(3))
                    .expect("matched health response")
            })
        })
        .collect();

    let mut request_ids = workers
        .into_iter()
        .map(|worker| {
            worker.join().expect("request thread")["request_id"]
                .as_str()
                .expect("response request ID")
                .to_owned()
        })
        .collect::<Vec<_>>();
    request_ids.sort();
    request_ids.dedup();
    assert_eq!(request_ids.len(), 4);

    let shutdown = client
        .shutdown(Duration::from_secs(2))
        .expect("graceful concurrent-engine shutdown");
    assert!(!shutdown.forced);
}

#[test]
fn structured_engine_errors_cross_the_boundary() {
    let client = real_client();
    let error = client
        .request("engine.missing", json!({}), Duration::from_secs(3))
        .expect_err("unknown method must return an engine error");
    match error {
        ClientError::Engine {
            request_id,
            code,
            retryable,
            details,
            ..
        } => {
            assert!(request_id.starts_with("rust-"));
            assert_eq!(code, "method_not_found");
            assert!(!retryable);
            assert_eq!(details.expect("method details")["method"], "engine.missing");
        }
        other => panic!("unexpected client error: {other}"),
    }
}
