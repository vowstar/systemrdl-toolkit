#!/usr/bin/env python3
"""
Check the elaborator against the worked examples in the SystemRDL 2.0 standard.

The standard states the resulting addresses and bit positions for several
examples. Those values are copied into test/test_spec_*.rdl as SPEC-EXPECT
lines, and this script compares them against what the elaborator produces.

Nothing here consults another implementation. The standard is the only oracle,
which is the point: a second implementation can tell you that two tools
disagree, but not which one is right.

    // SPEC-EXPECT-REG   <path>  <hex address>
    // SPEC-EXPECT-FIELD <path>  <msb>:<lsb>

Cases the toolkit is known not to satisfy are listed in KNOWN_FAILURES with the
clause they belong to. The list is self-cleaning: an entry that starts passing
is reported as an error so it gets removed.
"""

import glob
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

EXPECT_REG_RE = re.compile(r"SPEC-EXPECT-REG\s+(\S+)\s+(\S+)")
EXPECT_FIELD_RE = re.compile(r"SPEC-EXPECT-FIELD\s+(\S+)\s+(\d+):(\d+)")
CASE_RE = re.compile(r"SPEC-CASE\s+(.+)")

# Spec examples the elaborator does not satisfy yet. Keyed by test file name,
# valued by the clause and a short description.
KNOWN_FAILURES = {}


def parse_expectations(path):
    """Return (case_name, [(kind, path, expected)]) from the SPEC-EXPECT lines."""
    case = ""
    expectations = []
    with open(path, "r", encoding="utf-8") as handle:
        for line in handle:
            case_match = CASE_RE.search(line)
            if case_match and not case:
                case = case_match.group(1).strip()
            reg_match = EXPECT_REG_RE.search(line)
            if reg_match:
                expectations.append(("reg", reg_match.group(1), int(reg_match.group(2), 16)))
                continue
            field_match = EXPECT_FIELD_RE.search(line)
            if field_match:
                expectations.append(("field", field_match.group(1), (int(field_match.group(2)), int(field_match.group(3)))))
    return case, expectations


def elaborate(elaborator, rdl_path):
    """Return the simplified model, or None if elaboration failed."""
    handle, json_path = tempfile.mkstemp(suffix=".json")
    os.close(handle)
    try:
        result = subprocess.run(
            [elaborator, rdl_path, "--json=%s" % json_path],
            capture_output=True,
            text=True,
            timeout=30,
        )
        if result.returncode != 0:
            return None
        with open(json_path, "r", encoding="utf-8") as fh:
            return json.load(fh)
    except Exception:
        return None
    finally:
        if os.path.exists(json_path):
            os.unlink(json_path)


def build_actual(model):
    """Return {path: value} for registers and fields in the simplified model."""
    actual = {}
    for register in model.get("registers", []):
        parts = list(register.get("path", [])) + [register.get("inst_name", "")]
        reg_path = ".".join(part for part in parts if part)
        try:
            actual[("reg", reg_path)] = int(str(register.get("absolute_address", "0x0")), 16)
        except ValueError:
            pass
        for field in register.get("fields", []):
            if field.get("reserved") is True:
                continue
            key = ("field", "%s.%s" % (reg_path, field.get("inst_name", "")))
            actual[key] = (field.get("msb"), field.get("lsb"))
    return actual


def check_file(elaborator, rdl_path):
    """Return (case, [failure strings])."""
    case, expectations = parse_expectations(rdl_path)
    if not expectations:
        return case, ["no SPEC-EXPECT lines found"]

    model = elaborate(elaborator, rdl_path)
    if model is None:
        return case, ["elaboration failed"]

    actual = build_actual(model)
    failures = []
    for kind, path, expected in expectations:
        got = actual.get((kind, path))
        if got is None:
            failures.append("%s %s: expected %s, not present in the model" % (kind, path, fmt(kind, expected)))
        elif got != expected:
            failures.append("%s %s: expected %s, got %s" % (kind, path, fmt(kind, expected), fmt(kind, got)))
    return case, failures


def fmt(kind, value):
    if kind == "reg":
        return hex(value)
    return "%d:%d" % value


def main():
    root = Path(__file__).resolve().parent.parent
    elaborator = str(root / "build" / "systemrdl_elaborator")
    if not os.path.exists(elaborator):
        print("[FAIL] elaborator not found: %s" % elaborator)
        return 1

    cases = sorted(glob.glob(str(root / "test" / "test_spec_*.rdl")))
    if not cases:
        print("[FAIL] no test/test_spec_*.rdl files found")
        return 1

    print("[INFO] checking %d spec example(s) against SystemRDL 2.0" % len(cases))
    print("=" * 78)

    errors = 0
    for rdl_path in cases:
        name = os.path.basename(rdl_path)
        case, failures = check_file(elaborator, rdl_path)
        known = KNOWN_FAILURES.get(name)

        if failures and known:
            print("[KNOWN] %s (%s)" % (name, case))
            print("        %s" % known)
            for failure in failures:
                print("        %s" % failure)
        elif failures:
            errors += 1
            print("[FAIL]  %s (%s)" % (name, case))
            for failure in failures:
                print("        %s" % failure)
        elif known:
            errors += 1
            print("[FAIL]  %s (%s) now conforms" % (name, case))
            print("        remove it from KNOWN_FAILURES")
        else:
            print("[OK]    %s (%s)" % (name, case))

    print("=" * 78)
    if errors:
        print("[FAIL] %d spec example(s) in an unexpected state" % errors)
        return 1
    print("[OK] every spec example is either conforming or a recorded non-conformance")
    return 0


if __name__ == "__main__":
    sys.exit(main())
