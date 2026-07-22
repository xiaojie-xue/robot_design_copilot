export const JOINT_COUNT = 7 as const;

export type JointVector = [number, number, number, number, number, number, number];

export type JointValidation =
  | { ok: true; value: JointVector }
  | { ok: false; field: number; message: string };

export function parseJointFields(fields: readonly string[]): JointValidation {
  if (fields.length !== JOINT_COUNT) {
    return {
      ok: false,
      field: 0,
      message: `Expected ${JOINT_COUNT} joint positions.`,
    };
  }

  const values: number[] = [];
  for (let index = 0; index < fields.length; index += 1) {
    const text = fields[index]?.trim() ?? "";
    const value = Number(text);
    if (text.length === 0 || !Number.isFinite(value)) {
      return {
        ok: false,
        field: index,
        message: `Joint ${index + 1} must be a finite number in radians.`,
      };
    }
    values.push(value);
  }

  return { ok: true, value: values as JointVector };
}

export function formatEngineeringValue(value: number): string {
  if (Object.is(value, -0)) {
    return "0.000000";
  }
  return value.toFixed(6);
}
