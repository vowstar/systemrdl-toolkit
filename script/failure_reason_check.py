#!/usr/bin/env python3
"""
Check that each expected-failure fixture fails for the rule it declares.

A fixture whose name ends in _fail states what it expects to break, one
EXPECT_ELABORATION_FAILURE line per rule:

    // EXPECT_ELABORATION_FAILURE: Field overlap detected
    // EXPECT_ELABORATION_FAILURE: exceeds register width

Each declared fragment must appear in the output of the tool that is supposed
to reject the fixture, and every error that tool reports must match one of the
declared fragments. A fixture that stops failing, or that starts failing for an
undeclared reason, is an error here: a marker nothing reads is prose, and the
rule the fixture was written for can rot away while the fixture still passes.

RDL fixtures are elaborated, RCSV fixtures are converted. The per-fixture
WILL_FAIL tests still check the exit status; this checks the claim.

Nothing here consults another implementation. The declared fragment is matched
against what this toolkit prints, so a reworded message makes this fail until
the fixture is updated with it.
"""

import glob
import os
import re
import subprocess
import sys
from pathlib import Path

MARKER_RE = re.compile(r"EXPECT_ELABORATION_FAILURE\s*[:\-]\s*(.+)")

# How each tool reports a single problem.
#   rdl  - the elaborator prints one "Line <a>:<b> - <text>" per error
#   rcsv - the converter prints "Error: <text>", the prefix added by the tool
PROBLEM_RE = {
    "rdl": re.compile(r"^\s*Line \d+:\d+ - (.+)$", re.MULTILINE),
    "rcsv": re.compile(r"^Error:\s*(.+)$", re.MULTILINE),
}


def parse_declared(path):
    """Return the fragments declared by the EXPECT_ELABORATION_FAILURE lines."""
    declared = []
    with open(path, "r", encoding="utf-8") as handle:
        for line in handle:
            match = MARKER_RE.search(line)
            if match:
                declared.append(match.group(1).strip())
    return declared


def run_fixture(kind, tool, path):
    """Return (returncode, combined output) for one fixture."""
    command = [tool, path]
    if kind == "rcsv":
        # Write anywhere but the source tree; the fixture is expected to be
        # rejected before an output file matters.
        command += ["-o", os.devnull]
    result = subprocess.run(command, capture_output=True, text=True, timeout=60)
    return result.returncode, result.stdout + result.stderr


def check_fixture(kind, tool, path):
    """Return (declared_count, reported_count, [failure strings])."""
    declared = parse_declared(path)
    if not declared:
        return 0, 0, ["no EXPECT_ELABORATION_FAILURE line found"]

    returncode, output = run_fixture(kind, tool, path)
    if returncode == 0:
        return len(declared), 0, ["elaboration succeeded; this is no longer a failure case"]

    reported = [match.strip() for match in PROBLEM_RE[kind].findall(output)]
    if not reported:
        return len(declared), 0, ["the tool failed without reporting a rule"]

    failures = []
    low_output = [entry.lower() for entry in reported]
    for fragment in declared:
        if not any(fragment.lower() in entry for entry in low_output):
            failures.append("declared but not reported: %r" % fragment)
    for entry in reported:
        if not any(fragment.lower() in entry.lower() for fragment in declared):
            failures.append("reported but not declared: %s" % entry)

    return len(declared), len(reported), failures


def main():
    root = Path(__file__).resolve().parent.parent
    tools = {
        "rdl": str(root / "build" / "systemrdl_elaborator"),
        "rcsv": str(root / "build" / "systemrdl_csv2rdl"),
    }
    for kind, tool in sorted(tools.items()):
        if not os.path.exists(tool):
            print("[FAIL] tool not found: %s" % tool)
            return 1

    fixtures = []
    for suffix, kind in ((".rdl", "rdl"), (".csv", "rcsv")):
        for path in sorted(glob.glob(str(root / "test" / ("*_fail" + suffix)))):
            fixtures.append((kind, path))
    if not fixtures:
        print("[FAIL] no test/*_fail fixtures found")
        return 1

    print("[INFO] checking %d expected-failure fixture(s)" % len(fixtures))
    print("=" * 78)

    errors = 0
    for kind, path in fixtures:
        name = os.path.basename(path)
        declared, reported, failures = check_fixture(kind, tools[kind], path)
        if failures:
            errors += 1
            print("[FAIL]  %s" % name)
            for failure in failures:
                print("        %s" % failure)
        else:
            print("[OK]    %s (%d declared rule(s), %d reported error(s))" % (name, declared, reported))

    print("=" * 78)
    if errors:
        print("[FAIL] %d expected-failure fixture(s) in an unexpected state" % errors)
        return 1
    print("[OK] every failure fixture reports exactly the rules it declares")
    return 0


if __name__ == "__main__":
    sys.exit(main())
