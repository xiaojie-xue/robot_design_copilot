import { describe, expect, it } from "vitest";

import { normalizeCommandError } from "./engine";

describe("normalizeCommandError", () => {
  it("preserves structured desktop command errors", () => {
    expect(
      normalizeCommandError({
        code: "engine_timeout",
        message: "Engine request request-7 timed out.",
        retryable: true,
        details: { request_id: "request-7" },
      }),
    ).toEqual({
      code: "engine_timeout",
      message: "Engine request request-7 timed out.",
      retryable: true,
      details: { request_id: "request-7" },
    });
  });

  it("does not trust a non-boolean retryable value", () => {
    expect(
      normalizeCommandError({
        code: "engine_protocol",
        message: "Malformed response.",
        retryable: "yes",
      }),
    ).toMatchObject({
      code: "engine_protocol",
      retryable: false,
    });
  });

  it("wraps unknown failures with a stable fallback code", () => {
    expect(normalizeCommandError(new Error("IPC unavailable"))).toEqual({
      code: "desktop_command_failed",
      message: "IPC unavailable",
      retryable: true,
    });
  });
});
