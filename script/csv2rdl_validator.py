#!/usr/bin/env python3
"""
CSV to SystemRDL Converter Validation Suite

This test suite validates the functionality of the systemrdl_csv2rdl converter
by testing various CSV input scenarios and verifying the generated SystemRDL output.
Supports both expected success and expected failure test cases.
"""

import glob
import re
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import List, Optional, Tuple


class CSV2RDLValidator:
    def __init__(self):
        """Initialize the validator with auto-detected paths."""
        # Get project root directory based on script location
        script_dir = Path(__file__).parent
        self.project_root = script_dir.parent

        # Set paths relative to project root
        self.test_dir = self.project_root / "test"
        self.csv2rdl_binary = self.project_root / "build" / "systemrdl_csv2rdl"
        self.parser_binary = self.project_root / "build" / "systemrdl_parser"
        self.elaborator_binary = self.project_root / "build" / "systemrdl_elaborator"
        self.temp_dir = None

        # Verify binaries exist
        if not self.csv2rdl_binary.exists():
            raise FileNotFoundError("CSV2RDL binary not found: {}".format(self.csv2rdl_binary))
        if not self.parser_binary.exists():
            raise FileNotFoundError("Parser binary not found: {}".format(self.parser_binary))
        if not self.elaborator_binary.exists():
            raise FileNotFoundError("Elaborator binary not found: {}".format(self.elaborator_binary))

        # Test results tracking
        self.results = {"passed": 0, "failed": 0, "errors": []}

    def setup_temp_dir(self):
        """Create temporary directory for test outputs."""
        self.temp_dir = Path(tempfile.mkdtemp(prefix="csv2rdl_test_"))
        print("[DIR] Using temporary directory: {}".format(self.temp_dir))

    def cleanup_temp_dir(self):
        """Remove temporary directory."""
        if self.temp_dir and self.temp_dir.exists():
            shutil.rmtree(self.temp_dir)
            print("[CLEAN] Cleaned up temporary directory")

    def run_csv2rdl(self, csv_file: Path, output_file: Optional[Path] = None) -> Tuple[bool, str, str]:
        """
        Run CSV2RDL converter on a CSV file.

        Args:
            csv_file: Input CSV file path
            output_file: Optional output RDL file path

        Returns:
            Tuple of (success, stdout, stderr)
        """
        cmd = [str(self.csv2rdl_binary), str(csv_file)]
        if output_file:
            cmd.extend(["-o", str(output_file)])

        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            return result.returncode == 0, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return False, "", "Process timed out"
        except Exception as e:
            return False, "", str(e)

    def run_parser(self, rdl_file: Path) -> Tuple[bool, str, str]:
        """
        Run SystemRDL parser on an RDL file to validate syntax.

        Args:
            rdl_file: RDL file to parse

        Returns:
            Tuple of (success, stdout, stderr)
        """
        cmd = [str(self.parser_binary), str(rdl_file)]

        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            return result.returncode == 0, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return False, "", "Parser timed out"
        except Exception as e:
            return False, "", str(e)

    def validate_rdl_content(self, rdl_file: Path, expected_patterns: List[str]) -> Tuple[bool, List[str]]:
        """
        Validate RDL file content against expected patterns.

        Args:
            rdl_file: RDL file to validate
            expected_patterns: List of regex patterns that should be found

        Returns:
            Tuple of (all_found, missing_patterns)
        """
        try:
            content = rdl_file.read_text()
            missing = []

            for pattern in expected_patterns:
                if not re.search(pattern, content, re.MULTILINE | re.DOTALL):
                    missing.append(pattern)

            return len(missing) == 0, missing
        except Exception as e:
            return False, ["Error reading file: {}".format(e)]

    def run_elaborator(self, rdl_file: Path) -> Tuple[bool, str, str, Optional[Path]]:
        """
        Run SystemRDL elaborator to generate simplified JSON.

        Args:
            rdl_file: RDL file to elaborate

        Returns:
            Tuple of (success, stdout, stderr, json_file_path)
        """
        cmd = [str(self.elaborator_binary), str(rdl_file), "-j"]

        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30, cwd=rdl_file.parent)

            # JSON file is created with _simplified.json suffix, need to check multiple possible names
            json_file1 = rdl_file.with_suffix(".rdl_simplified.json")  # file.rdl -> file.rdl_simplified.json
            json_file2 = rdl_file.with_name(rdl_file.stem + "_simplified.json")  # file.rdl -> file_simplified.json

            json_file = None
            if json_file1.exists():
                json_file = json_file1
            elif json_file2.exists():
                json_file = json_file2

            if result.returncode == 0 and json_file:
                return True, result.stdout, result.stderr, json_file
            else:
                return False, result.stdout, result.stderr, None

        except subprocess.TimeoutExpired:
            return False, "", "Elaborator timed out", None
        except Exception as e:
            return False, "", str(e), None

    def validate_json_reset_values(
        self, json_file: Path, expected_reset_values: List[Tuple[str, int]]
    ) -> Tuple[bool, List[str]]:
        """
        Validate JSON contains expected reset values.

        Args:
            json_file: JSON file to validate
            expected_reset_values: List of (field_name, reset_value) tuples

        Returns:
            Tuple of (all_found, missing_values)
        """
        try:
            import json

            with open(json_file, "r") as f:
                data = json.load(f)

            missing = []

            # Extract all fields from registers
            all_fields = []
            if "registers" in data:
                for reg in data["registers"]:
                    if "fields" in reg:
                        all_fields.extend(reg["fields"])

            # Check each expected reset value
            for field_name, expected_reset in expected_reset_values:
                found = False
                for field in all_fields:
                    if field.get("name") == field_name and field.get("reset") == expected_reset:
                        found = True
                        break

                if not found:
                    missing.append("Field '{}' with reset value {}".format(field_name, expected_reset))

            return len(missing) == 0, missing

        except Exception as e:
            return False, ["Error validating JSON: {}".format(e)]

    def is_expected_failure_test(self, csv_file: Path) -> bool:
        """
        Check if a CSV file is an expected failure test.

        Args:
            csv_file: CSV file to check

        Returns:
            True if this is an expected failure test
        """
        return csv_file.stem.endswith("_fail")

    def test_csv_file_success(self, csv_file: Path, test_name: str, expected_patterns: List[str] = None) -> bool:
        """
        Test a CSV file conversion that is expected to succeed.

        Args:
            csv_file: CSV file to test
            test_name: Name of the test for reporting
            expected_patterns: Optional regex patterns to validate in output

        Returns:
            True if test passed, False otherwise
        """
        print("\n[TEST] Testing: {} (Expected: SUCCESS)".format(test_name))
        print("   Input: {}".format(csv_file.name))

        # Generate output file path in temp directory
        output_file = self.temp_dir / "{}.rdl".format(csv_file.stem)

        # Step 1: Run CSV2RDL converter
        success, stdout, stderr = self.run_csv2rdl(csv_file, output_file)
        if not success:
            print("   [ERR] CSV2RDL conversion failed (unexpected)")
            print("      stdout: {}".format(stdout))
            print("      stderr: {}".format(stderr))
            self.results["errors"].append("{}: CSV2RDL conversion failed - {}".format(test_name, stderr))
            return False

        # Check if output file was created
        if not output_file.exists():
            print("   [ERR] Output file not created: {}".format(output_file))
            self.results["errors"].append("{}: Output file not created".format(test_name))
            return False

        print("   [OK] CSV2RDL conversion successful")
        print("   Output: {}".format(output_file.name))

        # Step 2: Validate RDL syntax with parser
        parse_success, parse_stdout, parse_stderr = self.run_parser(output_file)
        if not parse_success:
            print("   [FAIL] RDL syntax validation failed")
            print("      stdout: {}".format(parse_stdout))
            print("      stderr: {}".format(parse_stderr))
            self.results["errors"].append("{}: RDL syntax validation failed - {}".format(test_name, parse_stderr))
            return False

        print("   [OK] RDL syntax validation passed")

        # Step 3: Validate content patterns if provided
        if expected_patterns:
            content_valid, missing = self.validate_rdl_content(output_file, expected_patterns)
            if not content_valid:
                print("   [FAIL] Content validation failed")
                print("      Missing patterns: {}".format(missing))
                self.results["errors"].append(
                    "{}: Content validation failed - missing patterns: {}".format(test_name, missing)
                )
                return False

            print("   [OK] Content validation passed")

        # Step 4: Special JSON validation for reset values test
        if csv_file.stem == "test_csv_reset_values":
            elab_success, elab_stdout, elab_stderr, json_file = self.run_elaborator(output_file)
            if not elab_success:
                print("   [FAIL] JSON elaboration failed")
                print("      Command: {} {} -j".format(self.elaborator_binary, output_file))
                print("      stdout: {}".format(elab_stdout))
                print("      stderr: {}".format(elab_stderr))
                print("      Expected JSON file: {}".format(output_file.with_suffix(".rdl_simplified.json")))
                self.results["errors"].append(
                    "{}: JSON elaboration failed - {}".format(test_name, elab_stderr or "No error message")
                )
                return False

            print("   [OK] JSON elaboration successful")

            # Validate specific reset values in JSON
            expected_reset_values = [
                ("ZERO_FIELD", 0),
                ("ONE_FIELD", 1),
                ("HEX_FIELD", 171),  # 0xAB = 171
                ("DEC_FIELD", 123),
                ("WIDE_FIELD", 4660),  # 0x1234 = 4660
                ("SINGLE_BIT", 1),
            ]

            json_valid, missing_values = self.validate_json_reset_values(json_file, expected_reset_values)
            if not json_valid:
                print("   [FAIL] JSON reset value validation failed")
                print("      Missing reset values: {}".format(missing_values))
                self.results["errors"].append(
                    "{}: JSON reset value validation failed - missing: {}".format(test_name, missing_values)
                )
                return False

            print("   [OK] JSON reset value validation passed")

        print("   [OK] {} PASSED".format(test_name))
        return True

    def test_csv_file_failure(self, csv_file: Path, test_name: str, expected_error_patterns: List[str] = None) -> bool:
        """
        Test a CSV file conversion that is expected to fail.

        Args:
            csv_file: CSV file to test
            test_name: Name of the test for reporting
            expected_error_patterns: Optional regex patterns that should be found in error output

        Returns:
            True if test passed (failed as expected), False otherwise
        """
        print("\n[TEST] Testing: {} (Expected: FAILURE)".format(test_name))
        print("   Input: {}".format(csv_file.name))

        # Generate output file path in temp directory
        output_file = self.temp_dir / "{}.rdl".format(csv_file.stem)

        # Step 1: Run CSV2RDL converter - should fail
        success, stdout, stderr = self.run_csv2rdl(csv_file, output_file)
        if success:
            print("   [FAIL] CSV2RDL conversion succeeded (unexpected)")
            print("      Expected failure but got success")
            self.results["errors"].append("{}: Expected failure but conversion succeeded".format(test_name))
            return False

        print("   [OK] CSV2RDL conversion failed as expected")
        print("   Error: {}".format(stderr.strip() if stderr else stdout.strip()))

        # Step 2: Validate error patterns if provided
        if expected_error_patterns:
            error_output = stderr + stdout  # Check both stderr and stdout
            for pattern in expected_error_patterns:
                if re.search(pattern, error_output, re.MULTILINE | re.IGNORECASE):
                    print("   [OK] Found expected error pattern: {}".format(pattern))
                else:
                    print("   [FAIL] Missing expected error pattern: {}".format(pattern))
                    self.results["errors"].append("{}: Missing expected error pattern: {}".format(test_name, pattern))
                    return False

        print("   [OK] {} PASSED (failed as expected)".format(test_name))
        return True

    def test_csv_file(self, csv_file: Path, test_name: str, expected_patterns: List[str] = None) -> bool:
        """
        Test a single CSV file conversion (auto-detects expected success/failure).

        Args:
            csv_file: CSV file to test
            test_name: Name of the test for reporting
            expected_patterns: Optional patterns (content patterns for success, error patterns for failure)

        Returns:
            True if test passed, False otherwise
        """
        if self.is_expected_failure_test(csv_file):
            # For failure tests, expected_patterns are error patterns
            expected_error_patterns = expected_patterns or [
                r"error|Error|ERROR",  # Generic error indicators
                r"failed|Failed|FAILED",  # Generic failure indicators
            ]
            return self.test_csv_file_failure(csv_file, test_name, expected_error_patterns)
        else:
            # For success tests, expected_patterns are content patterns
            return self.test_csv_file_success(csv_file, test_name, expected_patterns)

    def run_basic_example_test(self):
        """Test basic example CSV file."""
        csv_file = self.test_dir / "test_csv_basic_example.csv"
        expected_patterns = [
            r"addrmap DEMO \{",
            r'name = "DEMO"',
            r'reg \{[^}]*name = "CTRL"',
            r'field \{[^}]*name = "ENABLE"',
            r'field \{[^}]*name = "MODE"',
            r'desc = "Operation mode selection[^"]*- 0x0: Mode0[^"]*- 0x1: Mode1[^"]*"',
            r"sw = rw",
            r"hw = rw",
        ]

        return self.test_csv_file(csv_file, "Basic Example Test", expected_patterns)

    def run_failure_tests(self):
        """Test expected failure cases."""
        test_files = [
            (
                "test_csv_mixed_types_fail.csv",
                "Mixed Types Failure Test",
                [r"mixed information types", r"Line 2 contains mixed information types"],
            ),
            (
                "test_csv_field_before_reg_fail.csv",
                "Field Before Register Failure Test",
                [r"field.*but no register", r"Line 3 defines a field but no register was defined"],
            ),
        ]

        results = []
        for csv_name, test_name, expected_error_patterns in test_files:
            csv_file = self.test_dir / csv_name
            if csv_file.exists():
                result = self.test_csv_file(csv_file, test_name, expected_error_patterns)
                results.append(result)
            else:
                print("   [WARNING]  Skipping {}: file not found".format(test_name))

        return all(results) if results else False

    def run_multiline_tests(self):
        """Test various multiline scenarios."""
        test_files = [
            ("test_csv_basic_multiline.csv", "Basic Multiline Test"),
            ("test_csv_advanced_multiline.csv", "Advanced Multiline Test"),
            ("test_csv_complex_multiline.csv", "Complex Multiline Test"),
            ("test_csv_realistic_multiline.csv", "Realistic Multiline Test"),
            ("test_csv_extreme_multiline.csv", "Extreme Multiline Test"),
        ]

        results = []
        for csv_name, test_name in test_files:
            csv_file = self.test_dir / csv_name
            if csv_file.exists():
                # For multiline tests, verify that names don't contain newlines
                expected_patterns = [
                    r"addrmap [A-Z_]+ \{",  # addrmap name without newlines
                    r'name = "[^"\n]+"',  # name attributes without newlines
                    r'desc = "[^"]*"',  # desc can contain newlines
                ]
                result = self.test_csv_file(csv_file, test_name, expected_patterns)
                results.append(result)
            else:
                print("   [WARNING]  Skipping {}: file not found".format(test_name))

        return all(results) if results else False

    def run_delimiter_test(self):
        """Test semicolon delimiter detection."""
        csv_file = self.test_dir / "test_csv_semicolon_delimiter.csv"
        expected_patterns = [r"addrmap \w+ \{", r'name = "[^"]+"']

        return self.test_csv_file(csv_file, "Semicolon Delimiter Test", expected_patterns)

    def run_fuzzy_matching_test(self):
        """Test fuzzy header matching."""
        csv_file = self.test_dir / "test_csv_fuzzy_header_matching.csv"
        expected_patterns = [
            r"addrmap \w+ \{",
            r"field \{[^}]*sw = \w+",  # Verify access properties are mapped
            r"field \{[^}]*hw = \w+",
        ]

        return self.test_csv_file(csv_file, "Fuzzy Header Matching Test", expected_patterns)

    def run_quote_handling_tests(self):
        """Test quote handling scenarios."""
        test_files = [
            ("test_csv_quotes_mixed.csv", "Mixed Quotes Test"),
            ("test_csv_quotes_single_only.csv", "Single Quotes Only Test"),
            ("test_csv_quotes_boundaries.csv", "Quote Boundaries Test"),
        ]

        results = []
        for csv_name, test_name in test_files:
            csv_file = self.test_dir / csv_name
            if csv_file.exists():
                # For quote tests, verify basic structure
                expected_patterns = [
                    r"addrmap \w+ \{",
                    r'name = "[^"]+"',
                    r"field \{",
                ]
                result = self.test_csv_file(csv_file, test_name, expected_patterns)
                results.append(result)
            else:
                print("   [WARNING]  Skipping {}: file not found".format(test_name))

        return all(results) if results else False

    def discover_csv_test_files(self):
        """Discover all CSV test files in test directory."""
        pattern = str(self.test_dir / "test_csv_*.csv")
        csv_files = glob.glob(pattern)
        return [Path(f) for f in sorted(csv_files)]

    def run_all_discovered_tests(self):
        """Run tests on all discovered CSV files."""
        csv_files = self.discover_csv_test_files()
        if not csv_files:
            print("[FAIL] No test CSV files found in {}".format(self.test_dir))
            return False

        print("[INFO] Found {} CSV test files".format(len(csv_files)))

        # Separate success and failure tests
        success_files = [f for f in csv_files if not self.is_expected_failure_test(f)]
        failure_files = [f for f in csv_files if self.is_expected_failure_test(f)]

        print("   [SUCCESS] Success test files: {}".format(len(success_files)))
        print("   [FAIL] Failure test files: {}".format(len(failure_files)))

        results = []

        # Test success cases
        for csv_file in success_files:
            test_name = "Auto-discovered Success: {}".format(csv_file.stem)

            # Basic validation patterns for success CSV files
            expected_patterns = [
                r"addrmap \w+ \{",  # Should have addrmap
                r'name = "[^"]+"',  # Should have name attributes
                r"\};\s*$",  # Should end properly
            ]

            # Special validation for reset values test
            if csv_file.stem == "test_csv_reset_values":
                expected_patterns.extend(
                    [
                        r"ZERO_FIELD\[0:0\] = 0;",  # Zero reset value
                        r"ONE_FIELD\[1:1\] = 1;",  # One reset value
                        r"HEX_FIELD\[15:8\] = 0xAB;",  # Hex reset value
                        r"DEC_FIELD\[23:16\] = 123;",  # Decimal reset value
                        r"WIDE_FIELD\[15:0\] = 0x1234;",  # Wide field hex
                        r"SINGLE_BIT\[16:16\] = 1;",  # Single bit reset
                    ]
                )

            result = self.test_csv_file(csv_file, test_name, expected_patterns)
            results.append(result)

        # Test failure cases
        for csv_file in failure_files:
            test_name = "Auto-discovered Failure: {}".format(csv_file.stem)
            # Generic error patterns for failure CSV files
            expected_error_patterns = [
                r"error|Error|ERROR",  # Should contain error
                r"Line \d+",  # Should report line numbers
            ]
            result = self.test_csv_file(csv_file, test_name, expected_error_patterns)
            results.append(result)

        return all(results)

    def run_validation_suite(self):
        """Run complete validation suite."""
        print("[START] Starting CSV2RDL Validation Suite")
        print("=" * 60)
        print("[DIR] Project root: {}".format(self.project_root))
        print("[DIR] Test directory: {}".format(self.test_dir))
        print("[TOOL] CSV2RDL binary: {}".format(self.csv2rdl_binary))
        print("[TOOL] Parser binary: {}".format(self.parser_binary))

        self.setup_temp_dir()

        try:
            # Run targeted test suites
            test_suites = [
                ("Basic Example", self.run_basic_example_test),
                ("Expected Failures", self.run_failure_tests),
                ("Multiline Processing", self.run_multiline_tests),
                ("Quote Handling", self.run_quote_handling_tests),
                ("Semicolon Delimiter", self.run_delimiter_test),
                ("Fuzzy Header Matching", self.run_fuzzy_matching_test),
                ("All Discovered CSV Files", self.run_all_discovered_tests),
            ]

            for suite_name, test_func in test_suites:
                try:
                    print("\n[DIR] Running {} Test Suite".format(suite_name))
                    print("-" * 50)

                    if test_func():
                        self.results["passed"] += 1
                        print("[OK] {} Test Suite PASSED".format(suite_name))
                    else:
                        self.results["failed"] += 1
                        print("[FAIL] {} Test Suite FAILED".format(suite_name))

                except Exception as e:
                    self.results["failed"] += 1
                    error_msg = "{} Test Suite encountered error: {}".format(suite_name, e)
                    self.results["errors"].append(error_msg)
                    print("[FATAL] {}".format(error_msg))

            # Print summary and return overall success
            return self.print_summary()

        finally:
            self.cleanup_temp_dir()

    def print_summary(self):
        """Print test results summary."""
        total = self.results["passed"] + self.results["failed"]

        print("\n" + "=" * 60)
        print("[SUMMARY] VALIDATION SUMMARY")
        print("=" * 60)
        print("Total Test Suites: {}".format(total))
        print("[OK] Passed: {}".format(self.results["passed"]))
        print("[FAIL] Failed: {}".format(self.results["failed"]))

        if self.results["errors"]:
            print("\n[INFO] Error Details:")
            for i, error in enumerate(self.results["errors"], 1):
                print("  {}. {}".format(i, error))

        success_rate = (self.results["passed"] / total * 100) if total > 0 else 0
        print("\n[RATE] Success Rate: {:.1f}%".format(success_rate))

        if self.results["failed"] == 0:
            print("[OK] All tests passed!")
            return True
        else:
            print("[ERROR] Some tests failed - please review the errors above")
            return False


def main():
    """Main entry point."""
    try:
        validator = CSV2RDLValidator()
        success = validator.run_validation_suite()

        # Exit with appropriate code
        exit_code = 0 if success else 1
        print("\n[EXIT] Exiting with code {}".format(exit_code))
        return exit_code

    except Exception as e:
        print("[FATAL] Fatal error: {}".format(e))
        return 1


if __name__ == "__main__":
    exit(main())
