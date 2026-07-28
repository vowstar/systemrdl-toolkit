#!/usr/bin/env python3
"""
Compare SystemRDL implementations: Python (systemrdl-compiler) vs C++ (systemrdl_elaborator)

Two independent comparisons are performed for each RDL file:

1. Status comparison: does each implementation accept or reject the file?
2. Value comparison: do the elaborated register addresses, register widths and
   field bit positions and reset values agree numerically?

The second comparison is the one that catches silently wrong numbers. A file
where both implementations report success but disagree on a reset value is a
defect, not a pass.
"""

import glob
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

try:
    from systemrdl import RDLCompiler
    from systemrdl.node import FieldNode, RegNode

    SYSTEMRDL_AVAILABLE = True
except ImportError:
    SYSTEMRDL_AVAILABLE = False


# Files whose elaborated values are known to disagree with the reference
# implementation. Each entry is a defect that predates this comparison, not an
# accepted difference.
#
# The baseline is self-cleaning: a file listed here that starts matching is
# reported as an error so the entry gets removed. Never add an entry to silence
# a new regression.
KNOWN_VALUE_MISMATCHES = {}


class ImplementationComparator:
    def __init__(self, test_dir="test"):
        # Get project root directory based on script location
        script_dir = Path(__file__).parent
        project_root = script_dir.parent

        # Set paths relative to project root
        self.test_dir = str(project_root / test_dir)
        self.cpp_exe = str(project_root / "build" / "systemrdl_elaborator")
        self.python_script = str(script_dir / "rdl_semantic_validator.py")

        self.results = {
            "cpp_only_pass": [],
            "python_only_pass": [],
            "both_pass": [],
            "both_fail": [],
            "different_errors": [],
            "cpp_fail_python_pass": [],
            "python_fail_cpp_pass": [],
            "value_mismatch": [],
            "value_match": [],
            "known_mismatch": [],
            "stale_baseline": [],
            "missing_nodes": [],
        }

    def check_expect_elaboration_failure(self, rdl_file):
        """Check if RDL file is marked as expecting elaboration failure"""
        try:
            # Method 1: Check filename for _fail suffix (new naming convention)
            file_basename = os.path.basename(rdl_file)
            file_stem = os.path.splitext(file_basename)[0]  # Remove .rdl extension
            if file_stem.endswith("_fail"):
                return True

            # Method 2: Check file content for EXPECT_ELABORATION_FAILURE marker (legacy method)
            with open(rdl_file, "r", encoding="utf-8") as f:
                # Check first few lines for EXPECT_ELABORATION_FAILURE marker
                for i, line in enumerate(f):
                    if i >= 10:  # Only check first 10 lines
                        break
                    if "EXPECT_ELABORATION_FAILURE" in line:
                        return True
                return False
        except Exception:
            return False

    def run_cpp_implementation(self, rdl_file):
        """Run C++ implementation and return (success, output)"""
        try:
            result = subprocess.run([self.cpp_exe, rdl_file], capture_output=True, text=True, timeout=10)
            return result.returncode == 0, result.stdout + result.stderr
        except Exception as e:
            return False, str(e)

    def run_python_implementation(self, rdl_file):
        """Run Python implementation and return (success, output)"""
        try:
            result = subprocess.run(
                [sys.executable, self.python_script, rdl_file],
                capture_output=True,
                text=True,
                timeout=10,
            )
            return result.returncode == 0, result.stdout + result.stderr
        except Exception as e:
            return False, str(e)

    @staticmethod
    def normalize_reset(value):
        """Normalize a reset value to int.

        The C++ simplified JSON emits reset as a plain number in format 1.0 and
        as a hex string in format 2.0. Both are accepted so this comparison
        works across the format change.
        """
        if value is None:
            return None
        if isinstance(value, bool):
            return int(value)
        if isinstance(value, int):
            return value
        if isinstance(value, str):
            text = value.strip().lower()
            if not text:
                return None
            try:
                if text.startswith("0x"):
                    return int(text, 16)
                return int(text, 10)
            except ValueError:
                return value
        return value

    def build_golden_model(self, rdl_file):
        """Build the reference value model using the official systemrdl-compiler.

        Returns a dict keyed by hierarchical path, or None if the reference
        implementation cannot elaborate the file.
        """
        if not SYSTEMRDL_AVAILABLE:
            return None
        try:
            rdlc = RDLCompiler()
            rdlc.compile_file(rdl_file)
            root = rdlc.elaborate()
        except Exception:
            return None

        model = {}
        try:
            for node in root.descendants(unroll=True):
                path = node.get_path()
                if isinstance(node, RegNode):
                    model[path] = {
                        "kind": "reg",
                        "address": node.absolute_address,
                        "width": node.get_property("regwidth"),
                    }
                elif isinstance(node, FieldNode):
                    # high and low are the normalised positions. msb and lsb
                    # follow the register's bit ordering, so they are swapped
                    # for an msb0 register and would not line up with the
                    # positions this toolkit reports.
                    model[path] = {
                        "kind": "field",
                        "msb": node.high,
                        "lsb": node.low,
                        "width": node.width,
                        "reset": self.normalize_reset(node.get_property("reset")),
                    }
        except Exception:
            return None
        return model

    def build_cpp_model(self, rdl_file):
        """Build the value model from the C++ elaborator simplified JSON output.

        Auto-generated reserved fields are skipped: they are a documented
        C++-only feature with no counterpart in the reference implementation.
        """
        json_fd, json_path = tempfile.mkstemp(suffix=".json")
        os.close(json_fd)
        try:
            result = subprocess.run(
                [self.cpp_exe, rdl_file, f"--json={json_path}"],
                capture_output=True,
                text=True,
                timeout=10,
            )
            if result.returncode != 0:
                return None
            with open(json_path, "r", encoding="utf-8") as handle:
                data = json.load(handle)
        except Exception:
            return None
        finally:
            if os.path.exists(json_path):
                os.unlink(json_path)

        model = {}
        for register in data.get("registers", []):
            path_parts = list(register.get("path", [])) + [register.get("inst_name", "")]
            reg_path = ".".join(part for part in path_parts if part)
            try:
                address = int(str(register.get("absolute_address", "0x0")), 16)
            except ValueError:
                address = None
            model[reg_path] = {
                "kind": "reg",
                "address": address,
                "width": register.get("register_width"),
            }
            for field in register.get("fields", []):
                if field.get("reserved") is True:
                    continue
                field_path = f"{reg_path}.{field.get('inst_name', '')}"
                model[field_path] = {
                    "kind": "field",
                    "msb": field.get("msb"),
                    "lsb": field.get("lsb"),
                    "width": field.get("width"),
                    "reset": self.normalize_reset(field.get("reset")),
                }
        return model

    def compare_values(self, rdl_file, file_name):
        """Compare elaborated numeric values against the reference implementation."""
        golden = self.build_golden_model(rdl_file)
        cpp_model = self.build_cpp_model(rdl_file)

        if golden is None or cpp_model is None:
            print("   [VALUE] Skipped (one side could not produce a model)")
            return

        compared_keys = {"reg": ("address", "width"), "field": ("msb", "lsb", "width", "reset")}

        mismatches = []
        for path in sorted(set(golden) & set(cpp_model)):
            golden_node = golden[path]
            cpp_node = cpp_model[path]
            if golden_node["kind"] != cpp_node["kind"]:
                continue
            for key in compared_keys[golden_node["kind"]]:
                golden_value = golden_node.get(key)
                cpp_value = cpp_node.get(key)
                if golden_value is None and cpp_value is None:
                    continue
                if golden_value != cpp_value:
                    mismatches.append((path, key, golden_value, cpp_value))

        missing = sorted(set(golden) - set(cpp_model))
        extra = sorted(set(cpp_model) - set(golden))

        known_reason = KNOWN_VALUE_MISMATCHES.get(file_name)

        if mismatches:
            if known_reason:
                self.results["known_mismatch"].append((file_name, mismatches, known_reason))
                print(f"   [VALUE] {len(mismatches)} known mismatch(es) [KNOWN]")
                print(f"      known defect: {known_reason}")
            else:
                self.results["value_mismatch"].append((file_name, mismatches))
                print(f"   [VALUE] {len(mismatches)} numeric mismatch(es) [FAIL]")
            for path, key, golden_value, cpp_value in mismatches[:10]:
                print(f"      {path}.{key}: reference={golden_value} cpp={cpp_value}")
            if len(mismatches) > 10:
                print(f"      ... and {len(mismatches) - 10} more")
        else:
            if known_reason:
                self.results["stale_baseline"].append(file_name)
                print("   [VALUE] All values match, but file is listed in KNOWN_VALUE_MISMATCHES [FAIL]")
                print("      remove the entry from KNOWN_VALUE_MISMATCHES")
            else:
                self.results["value_match"].append(file_name)
                print("   [VALUE] All compared values match [OK]")

        if missing or extra:
            self.results["missing_nodes"].append((file_name, missing, extra))
            if missing:
                print(f"   [VALUE] {len(missing)} node(s) present in reference but absent in C++ [WARNING]")
                for path in missing[:5]:
                    print(f"      missing: {path}")
            if extra:
                print(f"   [VALUE] {len(extra)} node(s) present in C++ but absent in reference [WARNING]")
                for path in extra[:5]:
                    print(f"      extra: {path}")

    def extract_error_messages(self, output):
        """Extract key error messages from output"""
        errors = []
        lines = output.split("\n")
        for line in lines:
            if (
                "error:" in line.lower()
                or "fatal:" in line.lower()
                or "field overlap detected" in line.lower()
                or "field exceeds" in line.lower()
                or "overlaps with" in line.lower()
            ):
                # Clean up the error message
                error = line.strip()
                # For C++ format, remove "Line X:Y - " prefix
                if " - " in error and "Line " in error:
                    error = error.split(" - ", 1)[1]
                # For Python format, extract after file:line:col
                elif ":" in error:
                    parts = error.split(":", 3)
                    if len(parts) >= 4:
                        error = parts[3].strip()
                errors.append(error)
        return errors

    def compare_file(self, rdl_file):
        """Compare results for a single file"""
        file_name = os.path.basename(rdl_file)
        expect_failure = self.check_expect_elaboration_failure(rdl_file)

        print(f"\n[FOLDER] Testing: {file_name}")
        if expect_failure:
            print("   [VAL] Expected: FAILURE (validation test)")
        else:
            print("   [VAL] Expected: SUCCESS")

        # Run both implementations
        cpp_success, cpp_output = self.run_cpp_implementation(rdl_file)
        python_success, python_output = self.run_python_implementation(rdl_file)

        print(f"   [CPP] C++ Result: {'[OK] PASS' if cpp_success else '[FAIL] FAIL'}")
        print(f"   [PY] Python Result: {'[OK] PASS' if python_success else '[FAIL] FAIL'}")

        # Compare elaborated values whenever both sides accepted the file.
        # Status agreement alone does not prove the numbers agree.
        if not expect_failure and cpp_success and python_success:
            self.compare_values(rdl_file, file_name)

        # For expected failures, invert the logic
        if expect_failure:
            cpp_success = not cpp_success
            python_success = not python_success
            print(f"   [VAL] C++ Validation: {'[OK] PASS' if cpp_success else '[FAIL] FAIL'}")
            print(f"   [VAL] Python Validation: {'[OK] PASS' if python_success else '[FAIL] FAIL'}")

        # Categorize results
        if cpp_success and python_success:
            self.results["both_pass"].append(file_name)
            print("   [SUMMARY] Status: BOTH PASS [OK]")
        elif not cpp_success and not python_success:
            # Check if error messages are similar
            cpp_errors = self.extract_error_messages(cpp_output)
            python_errors = self.extract_error_messages(python_output)

            if self.errors_similar(cpp_errors, python_errors):
                self.results["both_fail"].append(file_name)
                print("   [SUMMARY] Status: BOTH FAIL (similar errors) [WARNING]")
            else:
                self.results["different_errors"].append((file_name, cpp_errors, python_errors))
                print("   [SUMMARY] Status: BOTH FAIL (different errors) [WARNING]")
                print(f"      C++ errors: {cpp_errors}")
                print(f"      Python errors: {python_errors}")
        elif cpp_success and not python_success:
            self.results["cpp_only_pass"].append((file_name, python_output))
            print("   [SUMMARY] Status: C++ PASS, Python FAIL [WARNING]")
            print(f"      Python error: {self.extract_error_messages(python_output)}")
        elif not cpp_success and python_success:
            self.results["python_only_pass"].append((file_name, cpp_output))
            print("   [SUMMARY] Status: Python PASS, C++ FAIL [WARNING]")
            print(f"      C++ error: {self.extract_error_messages(cpp_output)}")

    def errors_similar(self, cpp_errors, python_errors):
        """Check if error messages are conceptually similar"""
        if not cpp_errors and not python_errors:
            return True
        if not cpp_errors or not python_errors:
            return False

        # Check for key error concepts
        cpp_concepts = set()
        python_concepts = set()

        for error in cpp_errors:
            if "overlap" in error.lower():
                cpp_concepts.add("overlap")
            if "exceed" in error.lower() or "boundary" in error.lower():
                cpp_concepts.add("boundary")
            if "power of 2" in error.lower():
                cpp_concepts.add("power_of_2")

        for error in python_errors:
            if "overlap" in error.lower():
                python_concepts.add("overlap")
            if "exceed" in error.lower() or "boundary" in error.lower():
                python_concepts.add("boundary")
            if "power of 2" in error.lower():
                python_concepts.add("power_of_2")

        return len(cpp_concepts.intersection(python_concepts)) > 0

    def run_comparison(self):
        """Run comparison on all RDL files"""
        if not os.path.exists(self.test_dir):
            print(f"[FAIL] Test directory does not exist: {self.test_dir}")
            return False

        rdl_files = glob.glob(os.path.join(self.test_dir, "*.rdl"))
        # Files reproducing worked examples from the standard are checked by
        # script/spec_conformance_check.py against the values the standard
        # states. Tracking them here as well would duplicate the bookkeeping.
        rdl_files = [f for f in rdl_files if not os.path.basename(f).startswith("test_spec_")]
        if not rdl_files:
            print(f"[FAIL] No RDL files found in directory {self.test_dir}")
            return False

        print(f"[VAL] Found {len(rdl_files)} RDL files for comparison")
        if not SYSTEMRDL_AVAILABLE:
            print("[FAIL] systemrdl-compiler is not importable; value comparison cannot run")
            print("       Install it with: pip install -r requirements.txt")
            return False
        print("=" * 80)

        # Test executables
        if not os.path.exists(self.cpp_exe):
            print(f"[FAIL] C++ executable not found: {self.cpp_exe}")
            return False

        if not os.path.exists(self.python_script):
            print(f"[FAIL] Python script not found: {self.python_script}")
            return False

        for rdl_file in sorted(rdl_files):
            self.compare_file(rdl_file)

        self.print_summary()
        return not self.results["value_mismatch"] and not self.results["stale_baseline"]

    def print_summary(self):
        """Print comparison summary"""
        print("\n" + "=" * 80)
        print("[SUMMARY] COMPARISON SUMMARY")
        print("=" * 80)

        total_files = (
            len(self.results["both_pass"])
            + len(self.results["both_fail"])
            + len(self.results["different_errors"])
            + len(self.results["cpp_only_pass"])
            + len(self.results["python_only_pass"])
        )

        print(f"[FOLDER] Total files tested: {total_files}")
        print(f"[OK] Both implementations pass: {len(self.results['both_pass'])}")
        print(f"[WARNING]  Both implementations fail (similar): {len(self.results['both_fail'])}")
        print(f"[WARNING]  Both implementations fail (different): {len(self.results['different_errors'])}")
        print(f"[CPP] C++ only passes: {len(self.results['cpp_only_pass'])}")
        print(f"[PY] Python only passes: {len(self.results['python_only_pass'])}")

        # Detailed breakdown
        if self.results["both_pass"]:
            print(f"\n[OK] BOTH PASS ({len(self.results['both_pass'])}):")
            for file_name in self.results["both_pass"]:
                print(f"   - {file_name}")

        if self.results["cpp_only_pass"]:
            print(f"\n[CPP] C++ ONLY PASS ({len(self.results['cpp_only_pass'])}):")
            for file_name, python_error in self.results["cpp_only_pass"]:
                print(f"   - {file_name}")
                errors = self.extract_error_messages(python_error)
                if errors:
                    print(f"     Python error: {errors[0]}")

        if self.results["python_only_pass"]:
            print(f"\n[PY] PYTHON ONLY PASS ({len(self.results['python_only_pass'])}):")
            for file_name, cpp_error in self.results["python_only_pass"]:
                print(f"   - {file_name}")
                errors = self.extract_error_messages(cpp_error)
                if errors:
                    print(f"     C++ error: {errors[0]}")

        if self.results["different_errors"]:
            print(f"\n[WARNING]  DIFFERENT ERROR TYPES ({len(self.results['different_errors'])}):")
            for file_name, cpp_errors, python_errors in self.results["different_errors"]:
                print(f"   - {file_name}")
                print(f"     C++: {cpp_errors}")
                print(f"     Python: {python_errors}")

        # Value comparison is the gate: matching exit status proves nothing
        # about the numbers that end up in RTL and firmware headers.
        print("\n[VALUE] NUMERIC VALUE COMPARISON")
        print(f"   [OK] Files with all values matching: {len(self.results['value_match'])}")
        print(f"   [FAIL] Files with new value mismatches: {len(self.results['value_mismatch'])}")
        print(f"   [KNOWN] Files with known value mismatches: {len(self.results['known_mismatch'])}")
        print(f"   [WARNING]  Files with node set differences: {len(self.results['missing_nodes'])}")

        if self.results["value_mismatch"]:
            print(f"\n[FAIL] NEW VALUE MISMATCHES ({len(self.results['value_mismatch'])}):")
            for file_name, mismatches in self.results["value_mismatch"]:
                print(f"   - {file_name}: {len(mismatches)} mismatch(es)")
                for path, key, golden_value, cpp_value in mismatches[:5]:
                    print(f"     {path}.{key}: reference={golden_value} cpp={cpp_value}")

        if self.results["known_mismatch"]:
            print(f"\n[KNOWN] KNOWN VALUE MISMATCHES ({len(self.results['known_mismatch'])}):")
            for file_name, mismatches, reason in self.results["known_mismatch"]:
                print(f"   - {file_name}: {len(mismatches)} mismatch(es)")
                print(f"     {reason}")

        if self.results["stale_baseline"]:
            print(f"\n[FAIL] STALE BASELINE ENTRIES ({len(self.results['stale_baseline'])}):")
            for file_name in self.results["stale_baseline"]:
                print(f"   - {file_name} now matches; remove it from KNOWN_VALUE_MISMATCHES")

        if self.results["missing_nodes"]:
            print(f"\n[WARNING]  NODE SET DIFFERENCES ({len(self.results['missing_nodes'])}):")
            for file_name, missing, extra in self.results["missing_nodes"]:
                print(f"   - {file_name}: {len(missing)} missing, {len(extra)} extra")

        # Analysis
        print("\n[INFO] ANALYSIS:")
        compatibility = len(self.results["both_pass"]) + len(self.results["both_fail"])
        compatibility_percent = (compatibility / total_files) * 100 if total_files > 0 else 0

        print(f"   [RATE] Compatibility: {compatibility}/{total_files} ({compatibility_percent:.1f}%)")

        if len(self.results["cpp_only_pass"]) > 0:
            print("   [CPP] C++ implementation may be more permissive")
        if len(self.results["python_only_pass"]) > 0:
            print("   [PY] Python implementation may be more permissive")
        if len(self.results["different_errors"]) > 0:
            print("   [WARNING]  Error message differences detected")


def main():
    if len(sys.argv) > 1:
        test_dir = sys.argv[1]
    else:
        test_dir = "test"

    comparator = ImplementationComparator(test_dir)
    success = comparator.run_comparison()
    if not success:
        print("\n[FAIL] Comparison failed: elaborated values disagree with the reference implementation")
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
