import { invoke, isTauri } from "@tauri-apps/api/core";

import type { JointVector } from "./domain/joints";

export interface EngineHealth {
  engineVersion: string;
  status: string;
  capabilities: string[];
  dependencies: Record<string, string>;
}

export interface ForwardKinematicsResult {
  engineVersion: string;
  modelId: string;
  baseFrame: string;
  toolFrame: string;
  jointCount: number;
  translationM: [number, number, number];
  orientationXyzw: [number, number, number, number];
}

export interface EngineCommandError {
  code: string;
  message: string;
  retryable: boolean;
  details?: Record<string, unknown>;
}

export function hasDesktopRuntime(): boolean {
  return isTauri();
}

export async function getEngineHealth(): Promise<EngineHealth> {
  return invoke<EngineHealth>("engine_health");
}

export async function restartEngine(): Promise<EngineHealth> {
  return invoke<EngineHealth>("restart_engine");
}

export async function computeForwardKinematics(
  jointPositionsRad: JointVector,
): Promise<ForwardKinematicsResult> {
  return invoke<ForwardKinematicsResult>("forward_kinematics", {
    request: { jointPositionsRad },
  });
}

export function normalizeCommandError(error: unknown): EngineCommandError {
  if (typeof error === "object" && error !== null) {
    const candidate = error as Partial<EngineCommandError>;
    if (typeof candidate.code === "string" && typeof candidate.message === "string") {
      return {
        code: candidate.code,
        message: candidate.message,
        retryable: candidate.retryable === true,
        details: candidate.details,
      };
    }
  }
  return {
    code: "desktop_command_failed",
    message: error instanceof Error ? error.message : String(error),
    retryable: true,
  };
}
