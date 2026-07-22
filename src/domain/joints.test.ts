import { describe, expect, it } from "vitest";

import { formatEngineeringValue, parseJointFields } from "./joints";

describe("parseJointFields", () => {
  it("accepts exactly seven finite radian values", () => {
    expect(parseJointFields(["0", "1", "-1", "0.5", "2", "3", "4"])).toEqual({
      ok: true,
      value: [0, 1, -1, 0.5, 2, 3, 4],
    });
  });

  it("identifies the first invalid joint", () => {
    expect(parseJointFields(["0", "1", "", "0", "0", "0", "0"])).toEqual({
      ok: false,
      field: 2,
      message: "Joint 3 must be a finite number in radians.",
    });
  });

  it("rejects a vector with the wrong size", () => {
    expect(parseJointFields(["0"])).toMatchObject({ ok: false });
  });
});

describe("formatEngineeringValue", () => {
  it("normalizes negative zero", () => {
    expect(formatEngineeringValue(-0)).toBe("0.000000");
  });
});
