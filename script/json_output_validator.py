#!/usr/bin/env python3
"""
Simplified JSON Output Validator and Tester for SystemRDL Toolkit

This script validates the simplified JSON output from systemrdl_elaborator,
and can also run end-to-end tests to generate and validate simplified JSON output.
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Dict, List, Optional


class JsonValidator:
    def __init__(self, verbose: bool = True):
        self.errors = []
        self.warnings = []
        self.verbose = verbose

    def log_error(self, msg: str):
        self.errors.append(msg)
        if self.verbose:
            print(f"ERROR: {msg}")

    def log_warning(self, msg: str):
        self.warnings.append(msg)
        if self.verbose:
            print(f"WARNING: {msg}")

    def log_success(self, msg: str):
        if self.verbose:
            print(f"PASS: {msg}")

    def log_info(self, msg: str):
        if self.verbose:
            print(f"INFO: {msg}")

    def validate_json_file(self, file_path: str) -> Optional[Dict[str, Any]]:
        """Validate that file exists and contains valid JSON"""
        path = Path(file_path)

        if not path.exists():
            self.log_error(f"JSON file does not exist: {file_path}")
            return None

        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
            self.log_success(f"Valid JSON file: {file_path}")
            return data
        except json.JSONDecodeError as e:
            self.log_error(f"Invalid JSON in {file_path}: {e}")
            return None
        except Exception as e:
            self.log_error(f"Error reading {file_path}: {e}")
            return None

    def validate_simplified_json(self, data: Dict[str, Any]) -> bool:
        """Validate simplified JSON structure"""
        required_fields = ["format", "version", "addrmap", "registers"]

        # Check required top-level fields
        for field in required_fields:
            if field not in data:
                self.log_error(f"Missing required field in simplified JSON: {field}")
                return False

        # Check format
        if data["format"] != "SystemRDL_SimplifiedModel":
            self.log_error(f"Invalid format: expected 'SystemRDL_SimplifiedModel', got '{data['format']}'")
            return False

        # Check version
        if not isinstance(data["version"], str):
            self.log_error("Version field must be a string")
            return False

        # Validate addrmap structure
        if not self.validate_addrmap(data["addrmap"]):
            return False

        # Validate registers array
        if not isinstance(data["registers"], list):
            self.log_error("Registers field must be an array")
            return False

        if len(data["registers"]) == 0:
            self.log_warning("Registers array is empty")

        # Validate each register
        for i, register in enumerate(data["registers"]):
            if not self.validate_register(register, f"registers[{i}]"):
                return False

        # Validate optional regfiles if present
        if "regfiles" in data:
            if not isinstance(data["regfiles"], list):
                self.log_error("Regfiles field must be an array")
                return False

            for i, regfile in enumerate(data["regfiles"]):
                if not self.validate_regfile(regfile, f"regfiles[{i}]"):
                    return False

        self.log_success("Simplified JSON structure is valid")
        return True

    def validate_addrmap(self, addrmap: Dict[str, Any]) -> bool:
        """Validate addrmap structure"""
        required_fields = ["inst_name", "absolute_address"]

        for field in required_fields:
            if field not in addrmap:
                self.log_error(f"Missing required field '{field}' in addrmap")
                return False

        # Validate address format
        addr = addrmap["absolute_address"]
        if isinstance(addr, str) and addr.startswith("0x"):
            try:
                int(addr, 16)
            except ValueError:
                self.log_error(f"Invalid hex address format '{addr}' in addrmap")
                return False
        elif not isinstance(addr, int):
            self.log_error(f"Address must be hex string or integer in addrmap")
            return False

        return True

    def validate_regfile(self, regfile: Dict[str, Any], path: str) -> bool:
        """Validate regfile structure"""
        required_fields = ["inst_name", "absolute_address", "path", "size"]

        for field in required_fields:
            if field not in regfile:
                self.log_error(f"Missing required field '{field}' in regfile at {path}")
                return False

        # Validate address format
        addr = regfile["absolute_address"]
        if isinstance(addr, str) and addr.startswith("0x"):
            try:
                int(addr, 16)
            except ValueError:
                self.log_error(f"Invalid hex address format '{addr}' at {path}")
                return False
        elif not isinstance(addr, int):
            self.log_error(f"Address must be hex string or integer at {path}")
            return False

        # Validate path
        if not isinstance(regfile["path"], list):
            self.log_error(f"Path must be an array at {path}")
            return False

        # Validate size
        if not isinstance(regfile["size"], int) or regfile["size"] < 0:
            self.log_error(f"Size must be non-negative integer at {path}")
            return False

        return True

    def validate_register(self, register: Dict[str, Any], path: str) -> bool:
        """Validate individual register"""
        if not isinstance(register, dict):
            self.log_error(f"Register at {path} must be an object")
            return False

        required_fields = ["inst_name", "absolute_address", "path", "path_abs", "fields"]
        for field in required_fields:
            if field not in register:
                self.log_error(f"Missing required field '{field}' in register at {path}")
                return False

        # Validate address format
        addr = register["absolute_address"]
        if isinstance(addr, str) and addr.startswith("0x"):
            try:
                int(addr, 16)
            except ValueError:
                self.log_error(f"Invalid hex address format '{addr}' at {path}")
                return False
        elif not isinstance(addr, int):
            self.log_error(f"Address must be hex string or integer at {path}")
            return False

        # Validate path arrays
        if not isinstance(register["path"], list) or not isinstance(register["path_abs"], list):
            self.log_error(f"Path and path_abs must be arrays at {path}")
            return False

        # Validate offset if present
        if "offset" in register and (not isinstance(register["offset"], int) or register["offset"] < 0):
            self.log_error(f"Offset must be non-negative integer at {path}")
            return False

        # Validate fields
        if not isinstance(register["fields"], list):
            self.log_error(f"Fields must be an array at {path}")
            return False

        if len(register["fields"]) == 0:
            self.log_warning(f"Fields array is empty at {path}")

        # Validate each field
        for i, field in enumerate(register["fields"]):
            if not self.validate_field(field, f"{path}.fields[{i}]"):
                return False

        # Validate register-specific fields (new functionality)
        if "register_width" in register:
            if not isinstance(register["register_width"], int) or register["register_width"] <= 0:
                self.log_error(f"register_width must be positive integer at {path}")
                return False

        if "register_reset_value" in register:
            reset_value = register["register_reset_value"]
            if not isinstance(reset_value, str):
                self.log_error(f"register_reset_value must be string at {path}")
                return False

            # Validate hex format: must start with "0x" and contain only valid hex digits
            if not reset_value.startswith("0x"):
                self.log_error(f"register_reset_value must start with '0x' at {path}, got '{reset_value}'")
                return False

            hex_part = reset_value[2:]  # Remove "0x" prefix
            if not hex_part:
                self.log_error(f"register_reset_value must contain hex digits after '0x' at {path}")
                return False

            # Check if all characters are valid hex digits (lowercase preferred)
            valid_hex_chars = set("0123456789abcdefABCDEF")
            for char in hex_part:
                if char not in valid_hex_chars:
                    self.log_error(f"register_reset_value contains invalid hex character '{char}' at {path}")
                    return False

            # Warn if uppercase hex is used (lowercase is preferred)
            if any(c in "ABCDEF" for c in hex_part):
                self.log_warning(f"register_reset_value uses uppercase hex at {path}, lowercase preferred")

            # Validate that reset value fits within register width if both are present
            if "register_width" in register:
                try:
                    reset_int = int(reset_value, 16)
                    max_value = (1 << register["register_width"]) - 1
                    if reset_int > max_value:
                        self.log_error(
                            f"register_reset_value {reset_value} exceeds register_width {register['register_width']} at {path}"
                        )
                        return False
                except ValueError:
                    self.log_error(f"register_reset_value '{reset_value}' is not valid hex at {path}")
                    return False

        return True

    def validate_field(self, field: Dict[str, Any], path: str) -> bool:
        """Validate individual field"""
        if not isinstance(field, dict):
            self.log_error(f"Field at {path} must be an object")
            return False

        required_fields = ["inst_name", "absolute_address", "lsb", "msb", "width"]
        for field_name in required_fields:
            if field_name not in field:
                self.log_error(f"Missing required field '{field_name}' in field at {path}")
                return False

        # Validate address format
        addr = field["absolute_address"]
        if isinstance(addr, str) and addr.startswith("0x"):
            try:
                int(addr, 16)
            except ValueError:
                self.log_error(f"Invalid hex address format '{addr}' at {path}")
                return False
        elif not isinstance(addr, int):
            self.log_error(f"Address must be hex string or integer at {path}")
            return False

        # Validate bit positions
        for bit_field in ["lsb", "msb", "width"]:
            if not isinstance(field[bit_field], int) or field[bit_field] < 0:
                self.log_error(f"{bit_field} must be non-negative integer at {path}")
                return False

        # Validate bit range consistency
        if field["msb"] < field["lsb"]:
            self.log_error(f"MSB ({field['msb']}) must be >= LSB ({field['lsb']}) at {path}")
            return False

        expected_width = field["msb"] - field["lsb"] + 1
        if field["width"] != expected_width:
            self.log_error(f"Width ({field['width']}) doesn't match bit range ({expected_width}) at {path}")
            return False

        return True


class JsonTester:
    def __init__(self, verbose: bool = True):
        self.validator = JsonValidator(verbose)
        self.verbose = verbose

    def check_executable(self, exe_path: str) -> bool:
        """Check if executable exists and is executable"""
        path = Path(exe_path)
        if not path.exists():
            self.validator.log_error(f"Executable not found: {exe_path}")
            return False
        if not os.access(path, os.X_OK):
            self.validator.log_error(f"File is not executable: {exe_path}")
            return False
        return True

    def check_rdl_file(self, rdl_path: str) -> bool:
        """Check if RDL file exists"""
        path = Path(rdl_path)
        if not path.exists():
            self.validator.log_error(f"RDL file not found: {rdl_path}")
            return False
        return True

    def check_expect_elaboration_failure(self, rdl_path: str) -> bool:
        """Check if RDL file is marked as expecting elaboration failure"""
        try:
            # Method 1: Check filename for _fail suffix
            file_basename = os.path.basename(rdl_path)
            file_stem = os.path.splitext(file_basename)[0]
            if file_stem.endswith("_fail"):
                return True

            # Method 2: Check file content for EXPECT_ELABORATION_FAILURE marker
            with open(rdl_path, "r", encoding="utf-8") as f:
                for i, line in enumerate(f):
                    if i >= 10:  # Only check first 10 lines
                        break
                    if "EXPECT_ELABORATION_FAILURE" in line:
                        return True
                return False
        except Exception:
            return False

    def run_command(self, cmd: List[str], cwd: str = None) -> bool:
        """Run command and return success status"""
        try:
            result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
            return result.returncode == 0
        except Exception as e:
            self.validator.log_error(f"Failed to run command {' '.join(cmd)}: {e}")
            return False

    def get_file_size(self, file_path: str) -> int:
        """Get file size in bytes"""
        try:
            return os.path.getsize(file_path)
        except (FileNotFoundError, OSError):
            return 0

    def run_end_to_end_test(self, elaborator_exe: str, rdl_file: str) -> bool:
        """Run complete end-to-end simplified JSON test"""

        # Convert to absolute paths
        elaborator_exe = os.path.abspath(elaborator_exe)
        rdl_file = os.path.abspath(rdl_file)

        # Check prerequisites
        if not all([self.check_executable(elaborator_exe), self.check_rdl_file(rdl_file)]):
            return False

        test_name = Path(rdl_file).stem
        expect_failure = self.check_expect_elaboration_failure(rdl_file)

        if expect_failure:
            if self.verbose:
                print(f"Testing validation for: {test_name} (expecting elaboration failure)")
        else:
            if self.verbose:
                print(f"Testing simplified JSON output for: {test_name}")

        # Create temporary directory
        with tempfile.TemporaryDirectory(prefix="json_test_") as temp_dir:
            temp_path = Path(temp_dir)

            # Test elaborator simplified JSON output with custom filename
            if self.verbose:
                print("  Testing elaborator simplified JSON output...")
            json_output = temp_path / f"{test_name}_simplified.json"
            elaborator_cmd = [elaborator_exe, rdl_file, f"--json={json_output}"]

            elaborator_success = self.run_command(elaborator_cmd)

            if expect_failure:
                # For validation tests, we expect elaboration to fail
                if elaborator_success and json_output.exists():
                    self.validator.log_error("Expected elaboration failure, but elaboration succeeded")
                    return False
                else:
                    self.validator.log_success("Elaboration failed as expected (validation test passed)")
                    if self.verbose:
                        print(f"Validation test completed successfully for: {test_name}")
                    return True
            else:
                # For normal tests, we expect elaboration to succeed
                if not elaborator_success:
                    self.validator.log_error("Elaborator failed to generate simplified JSON")
                    return False

                if not json_output.exists():
                    self.validator.log_error("Elaborator simplified JSON file not generated")
                    return False

                self.validator.log_success(f"Elaborator simplified JSON file generated: {json_output}")

                # Validate simplified JSON
                json_data = self.validator.validate_json_file(str(json_output))
                if not json_data or not self.validator.validate_simplified_json(json_data):
                    return False

                # Test default filename generation
                if self.verbose:
                    print("  Testing default filename generation...")

                # Test elaborator default filename
                elaborator_default_cmd = [elaborator_exe, rdl_file, "--json"]
                if not self.run_command(elaborator_default_cmd, cwd=temp_dir):
                    self.validator.log_error("Elaborator failed with default JSON filename")
                    return False

                default_json = temp_path / f"{test_name}_simplified.json"
                if default_json.exists():
                    self.validator.log_success("Default simplified JSON filename generated correctly")
                else:
                    self.validator.log_error("Default simplified JSON filename not generated")
                    return False

                # Check file size
                json_size = self.get_file_size(str(json_output))

                if json_size > 100:
                    self.validator.log_success(f"Simplified JSON has reasonable size: {json_size} bytes")
                else:
                    self.validator.log_warning(f"Simplified JSON seems too small: {json_size} bytes")

        self.validator.log_success(f"Simplified JSON test completed successfully for: {test_name}")
        return True


def main():
    parser = argparse.ArgumentParser(description="Validate and test SystemRDL simplified JSON output")

    # Validation mode arguments
    parser.add_argument("--json", help="Path to simplified JSON file")
    parser.add_argument("--rdl", help="Path to original RDL file (for context)")

    # End-to-end test mode arguments
    parser.add_argument("--test", action="store_true", help="Run end-to-end simplified JSON test")
    parser.add_argument("--elaborator", help="Path to systemrdl_elaborator executable")

    # Common options
    parser.add_argument("--strict", action="store_true", help="Treat warnings as errors")
    parser.add_argument("--quiet", action="store_true", help="Suppress output (show only errors)")

    args = parser.parse_args()

    # Get the project root directory based on script location
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    # Adjust relative paths to be relative to project root
    def adjust_path(path_arg):
        if not path_arg:
            return path_arg
        path = Path(path_arg)
        if path.is_absolute():
            return str(path)
        if str(path).startswith("../"):
            return str(script_dir / path)
        return str(project_root / path)

    args.elaborator = adjust_path(args.elaborator)
    args.rdl = adjust_path(args.rdl)
    args.json = adjust_path(args.json)

    # Determine mode
    if args.test:
        # End-to-end test mode
        if not all([args.elaborator, args.rdl]):
            print("ERROR: Test mode requires --elaborator and --rdl arguments")
            # Provide usage examples
            print("Example usage:")
            print(
                "  python script/json_output_validator.py --test "
                "--elaborator build/systemrdl_elaborator "
                "--rdl test/test_minimal.rdl"
            )
            print(
                "  python json_output_validator.py --test "
                "--elaborator ../build/systemrdl_elaborator "
                "--rdl ../test/test_minimal.rdl"
            )
            sys.exit(1)

        tester = JsonTester(verbose=not args.quiet)
        success = tester.run_end_to_end_test(args.elaborator, args.rdl)

        if not success or tester.validator.errors:
            print("\nTest FAILED")
            sys.exit(1)
        elif tester.validator.warnings and args.strict:
            print("\nTest FAILED (strict mode, warnings treated as errors)")
            sys.exit(1)
        else:
            print("\nTest PASSED")
            sys.exit(0)
    else:
        # Validation mode
        if not args.json:
            print("ERROR: Must specify --json, or use --test mode")
            sys.exit(1)

        validator = JsonValidator(verbose=not args.quiet)

        # Validate simplified JSON
        if not args.quiet:
            print(f"Validating simplified JSON: {args.json}")
        json_data = validator.validate_json_file(args.json)
        if json_data:
            validator.validate_simplified_json(json_data)

        # Summary
        if not args.quiet:
            print("\nValidation Summary:")
            print(f"   Errors: {len(validator.errors)}")
            print(f"   Warnings: {len(validator.warnings)}")

        if validator.errors:
            print("\nValidation FAILED")
            sys.exit(1)
        elif validator.warnings and args.strict:
            print("\nValidation FAILED (strict mode, warnings treated as errors)")
            sys.exit(1)
        else:
            print("\nValidation PASSED")
            sys.exit(0)


if __name__ == "__main__":
    main()
