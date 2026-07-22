import { useEffect, useId, useRef, useState, type FormEvent } from "react";

import { formatEngineeringValue, JOINT_COUNT, parseJointFields } from "./domain/joints";
import {
  computeForwardKinematics,
  getEngineHealth,
  hasDesktopRuntime,
  normalizeCommandError,
  restartEngine,
  type EngineCommandError,
  type EngineHealth,
  type ForwardKinematicsResult,
} from "./engine";

const ZERO_JOINTS = Array.from({ length: JOINT_COUNT }, () => "0");

type RequestState = "idle" | "running" | "succeeded" | "failed";

export function App() {
  const [jointFields, setJointFields] = useState<string[]>(ZERO_JOINTS);
  const [health, setHealth] = useState<EngineHealth | null>(null);
  const [result, setResult] = useState<ForwardKinematicsResult | null>(null);
  const [error, setError] = useState<EngineCommandError | null>(null);
  const [requestState, setRequestState] = useState<RequestState>("idle");
  const [healthPending, setHealthPending] = useState(false);
  const fieldId = useId();
  const fieldRefs = useRef<Array<HTMLInputElement | null>>([]);
  const desktopRuntime = hasDesktopRuntime();

  async function refreshHealth(restart = false) {
    if (!desktopRuntime) {
      return;
    }
    setHealthPending(true);
    setError(null);
    try {
      setHealth(restart ? await restartEngine() : await getEngineHealth());
    } catch (caught) {
      setHealth(null);
      setError(normalizeCommandError(caught));
    } finally {
      setHealthPending(false);
    }
  }

  useEffect(() => {
    void refreshHealth();
  }, []);

  function updateJoint(index: number, value: string) {
    setJointFields((current) =>
      current.map((joint, jointIndex) => (jointIndex === index ? value : joint)),
    );
  }

  async function submit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    setError(null);
    setResult(null);

    const validation = parseJointFields(jointFields);
    if (!validation.ok) {
      setRequestState("failed");
      setError({
        code: "invalid_joint_position",
        message: validation.message,
        retryable: false,
      });
      fieldRefs.current[validation.field]?.focus();
      return;
    }
    if (!desktopRuntime) {
      setRequestState("failed");
      setError({
        code: "desktop_runtime_unavailable",
        message: "Forward kinematics is available inside the Tauri desktop application.",
        retryable: false,
      });
      return;
    }

    setRequestState("running");
    try {
      const response = await computeForwardKinematics(validation.value);
      setResult(response);
      setRequestState("succeeded");
    } catch (caught) {
      setError(normalizeCommandError(caught));
      setRequestState("failed");
    }
  }

  const engineReady = health?.status === "ok";

  return (
    <main className="app-shell">
      <header className="topbar">
        <div className="brand-mark" aria-hidden="true">
          RC
        </div>
        <div className="brand-copy">
          <p className="eyebrow">M0 platform proof</p>
          <h1>Robot Design Copilot</h1>
        </div>
        <div className={`engine-pill ${engineReady ? "online" : "offline"}`} role="status">
          <span className="status-dot" aria-hidden="true" />
          {healthPending
            ? "Checking engine"
            : engineReady
              ? `Engine ${health.engineVersion}`
              : desktopRuntime
                ? "Engine unavailable"
                : "Web preview"}
        </div>
      </header>

      <section className="hero">
        <div>
          <p className="eyebrow accent">Reference arm · 7 DoF</p>
          <h2>Forward kinematics, across the real process boundary.</h2>
          <p className="hero-copy">
            Enter seven joint positions in radians. Rust owns the isolated engine process;
            Pinocchio computes the versioned <code>base_T_tool0</code> result.
          </p>
        </div>
        <dl className="contract-summary">
          <div>
            <dt>Input</dt>
            <dd>q₁…q₇ · rad</dd>
          </div>
          <div>
            <dt>Translation</dt>
            <dd>metres</dd>
          </div>
          <div>
            <dt>Quaternion</dt>
            <dd>[x, y, z, w]</dd>
          </div>
        </dl>
      </section>

      <div className="workspace-grid">
        <section className="panel input-panel" aria-labelledby="joint-heading">
          <div className="panel-heading">
            <div>
              <p className="panel-index">01 / Joint state</p>
              <h3 id="joint-heading">Joint positions</h3>
            </div>
            <button
              className="text-button"
              type="button"
              onClick={() => {
                setJointFields(ZERO_JOINTS);
                setResult(null);
                setError(null);
                setRequestState("idle");
              }}
            >
              Reset zero pose
            </button>
          </div>

          <form onSubmit={submit} noValidate>
            <div className="joint-grid">
              {jointFields.map((value, index) => (
                <label className="joint-field" key={`${fieldId}-${index}`}>
                  <span>
                    Joint {index + 1} <small>q{index + 1}</small>
                  </span>
                  <div className="number-input">
                    <input
                      ref={(element) => {
                        fieldRefs.current[index] = element;
                      }}
                      aria-label={`Joint ${index + 1} position in radians`}
                      inputMode="decimal"
                      value={value}
                      onChange={(event) => updateJoint(index, event.target.value)}
                    />
                    <span>rad</span>
                  </div>
                </label>
              ))}
            </div>

            <button className="primary-button" type="submit" disabled={requestState === "running"}>
              {requestState === "running" ? "Computing…" : "Compute base_T_tool0"}
            </button>
          </form>

          {error ? (
            <div className="error-callout" role="alert">
              <strong>{error.code}</strong>
              <span>{error.message}</span>
            </div>
          ) : null}
        </section>

        <section className="panel result-panel" aria-labelledby="result-heading">
          <div className="panel-heading">
            <div>
              <p className="panel-index">02 / Tool pose</p>
              <h3 id="result-heading">base_T_tool0</h3>
            </div>
            {result ? <span className="version-tag">{result.modelId}</span> : null}
          </div>

          {result ? (
            <div className="result-content">
              <div className="coordinate-card">
                <p>Translation · m</p>
                <div className="coordinate-row">
                  {result.translationM.map((value, index) => (
                    <div key={index}>
                      <span>{["x", "y", "z"][index]}</span>
                      <strong>{formatEngineeringValue(value)}</strong>
                    </div>
                  ))}
                </div>
              </div>
              <div className="coordinate-card">
                <p>Orientation · xyzw</p>
                <div className="coordinate-row quaternion">
                  {result.orientationXyzw.map((value, index) => (
                    <div key={index}>
                      <span>{["x", "y", "z", "w"][index]}</span>
                      <strong>{formatEngineeringValue(value)}</strong>
                    </div>
                  ))}
                </div>
              </div>
              <div className="result-footer">
                <span>{result.baseFrame}</span>
                <i aria-hidden="true" />
                <span>{result.toolFrame}</span>
                <small>{result.jointCount} joints · engine {result.engineVersion}</small>
              </div>
            </div>
          ) : (
            <div className="empty-result">
              <div className="axis-glyph" aria-hidden="true">
                <span className="axis-x">x</span>
                <span className="axis-y">y</span>
                <span className="axis-z">z</span>
              </div>
              <h4>No pose computed</h4>
              <p>Submit a valid seven-joint state to request a result from the C++ engine.</p>
            </div>
          )}
        </section>
      </div>

      <footer className="engine-strip">
        <div>
          <span>Protocol</span>
          <strong>v{health?.protocolVersion ?? 1}</strong>
        </div>
        <div>
          <span>Capabilities</span>
          <strong>{health?.capabilities.join(" · ") ?? "Awaiting desktop engine"}</strong>
        </div>
        <button
          className="secondary-button"
          type="button"
          disabled={!desktopRuntime || healthPending}
          onClick={() => void refreshHealth(true)}
        >
          Restart engine
        </button>
      </footer>
    </main>
  );
}
