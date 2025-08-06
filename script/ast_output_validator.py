#!/usr/bin/env python3
"""
AST JSON Output Validator and Tester for SystemRDL Toolkit

This script validates the AST JSON output from both systemrdl_parser and systemrdl_elaborator,
and can also run end-to-end tests to generate and validate AST JSON output.
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
            print(f"[ERROR] {msg}")

    def log_warning(self, msg: str):
        self.warnings.append(msg)
        if self.verbose:
            print(f"[WARNING] {msg}")

    def log_success(self, msg: str):
        if self.verbose:
            print(f"[OK] {msg}")

    def log_info(self, msg: str):
        if self.verbose:
            print(f"[INFO] {msg}")

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

    def validate_ast_json(self, data: Dict[str, Any]) -> bool:
        """Validate AST JSON structure"""
        required_fields = ["format", "version", "ast"]

        # Check required top-level fields
        for field in required_fields:
            if field not in data:
                self.log_error(f"Missing required field in AST JSON: {field}")
                return False

        # Check format
        if data["format"] != "SystemRDL_AST":
            self.log_error(f"Invalid format: expected 'SystemRDL_AST', got '{data['format']}'")
            return False

        # Check version
        if not isinstance(data["version"], str):
            self.log_error("Version field must be a string")
            return False

        # Check AST structure
        if not isinstance(data["ast"], list):
            self.log_error("AST field must be an array")
            return False

        if len(data["ast"]) == 0:
            self.log_warning("AST array is empty")

        # Validate AST nodes
        for i, node in enumerate(data["ast"]):
            if not self.validate_ast_node(node, f"ast[{i}]"):
                return False

        self.log_success("AST JSON structure is valid")
        return True

    def validate_ast_node(self, node: Dict[str, Any], path: str) -> bool:
        """Validate individual AST node"""
        if not isinstance(node, dict):
            self.log_error(f"AST node at {path} must be an object")
            return False

        required_fields = ["type"]
        for field in required_fields:
            if field not in node:
                self.log_error(f"Missing required field '{field}' in AST node at {path}")
                return False

        node_type = node.get("type")
        if node_type == "rule":
            rule_fields = [
                "rule_name",
                "text",
                "start_line",
                "start_column",
                "stop_line",
                "stop_column",
            ]
            for field in rule_fields:
                if field not in node:
                    self.log_error(f"Missing field '{field}' in rule node at {path}")
                    return False
        elif node_type == "terminal":
            terminal_fields = ["text", "line", "column"]
            for field in terminal_fields:
                if field not in node:
                    self.log_error(f"Missing field '{field}' in terminal node at {path}")
                    return False

        # Recursively validate children
        if "children" in node and isinstance(node["children"], list):
            for i, child in enumerate(node["children"]):
                if not self.validate_ast_node(child, f"{path}.children[{i}]"):
                    return False

        return True

    def validate_elaborated_json(self, data: Dict[str, Any]) -> bool:
        """Validate elaborated model JSON structure"""
        required_fields = ["format", "version", "model"]

        # Check required top-level fields
        for field in required_fields:
            if field not in data:
                self.log_error(f"Missing required field in elaborated JSON: {field}")
                return False

        # Check format
        if data["format"] != "SystemRDL_ElaboratedModel":
            self.log_error(f"Invalid format: expected 'SystemRDL_ElaboratedModel', got '{data['format']}'")
            return False

        # Check version
        if not isinstance(data["version"], str):
            self.log_error("Version field must be a string")
            return False

        # Check model structure
        if not isinstance(data["model"], list):
            self.log_error("Model field must be an array")
            return False

        if len(data["model"]) == 0:
            self.log_warning("Model array is empty")

        # Validate model nodes
        for i, node in enumerate(data["model"]):
            if not self.validate_elaborated_node(node, f"model[{i}]"):
                return False

        self.log_success("Elaborated model JSON structure is valid")
        return True

    def validate_elaborated_node(self, node: Dict[str, Any], path: str) -> bool:
        """Validate individual elaborated model node"""
        if not isinstance(node, dict):
            self.log_error(f"Elaborated node at {path} must be an object")
            return False

        required_fields = ["node_type", "inst_name", "absolute_address", "size"]
        for field in required_fields:
            if field not in node:
                self.log_error(f"Missing required field '{field}' in elaborated node at {path}")
                return False

        # Validate node_type
        valid_node_types = ["addrmap", "regfile", "reg", "field", "mem"]
        if node["node_type"] not in valid_node_types:
            self.log_warning(f"Unknown node_type '{node['node_type']}' at {path}")

        # Validate address format
        addr = node["absolute_address"]
        if isinstance(addr, str) and addr.startswith("0x"):
            try:
                int(addr, 16)
            except ValueError:
                self.log_error(f"Invalid hex address format '{addr}' at {path}")
                return False
        elif not isinstance(addr, int):
            self.log_error(f"Address must be hex string or integer at {path}")
            return False

        # Validate size
        if not isinstance(node["size"], int) or node["size"] < 0:
            self.log_error(f"Size must be non-negative integer at {path}")
            return False

        # Recursively validate children
        if "children" in node and isinstance(node["children"], list):
            for i, child in enumerate(node["children"]):
                if not self.validate_elaborated_node(child, f"{path}.children[{i}]"):
                    return False

        return True

    def validate_content_match(self, ast_data: Dict[str, Any], elaborated_data: Dict[str, Any], rdl_file: str) -> bool:
        """Validate that AST and elaborated model contain consistent information"""

        # Basic consistency checks
        if len(ast_data["ast"]) == 0 and len(elaborated_data["model"]) > 0:
            self.log_warning("AST is empty but elaborated model has content")

        if len(elaborated_data["model"]) == 0 and len(ast_data["ast"]) > 0:
            self.log_warning("Elaborated model is empty but AST has content")

        # Check for addrmap in both
        has_addrmap_ast = self.find_node_type_in_ast(ast_data["ast"], "addrmap")
        has_addrmap_elaborated = any(node.get("node_type") == "addrmap" for node in elaborated_data["model"])

        if has_addrmap_ast and not has_addrmap_elaborated:
            self.log_error("AST contains addrmap but elaborated model doesn't")
            return False

        self.log_success("Content consistency check passed")
        return True

    def find_node_type_in_ast(self, ast_nodes: List[Dict[str, Any]], node_type: str) -> bool:
        """Recursively search for node type in AST"""
        for node in ast_nodes:
            if node.get("rule_name") == node_type:
                return True
            if "children" in node:
                if self.find_node_type_in_ast(node["children"], node_type):
                    return True
        return False


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
            # Method 1: Check filename for _fail suffix (new naming convention)
            import os

            file_basename = os.path.basename(rdl_path)
            file_stem = os.path.splitext(file_basename)[0]  # Remove .rdl extension
            if file_stem.endswith("_fail"):
                return True

            # Method 2: Check file content for EXPECT_ELABORATION_FAILURE marker (legacy method)
            with open(rdl_path, "r", encoding="utf-8") as f:
                # Check first few lines for EXPECT_ELABORATION_FAILURE marker
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

    def run_end_to_end_test(self, parser_exe: str, elaborator_exe: str, rdl_file: str) -> bool:
        """Run complete end-to-end JSON test"""

        # Convert to absolute paths
        parser_exe = os.path.abspath(parser_exe)
        elaborator_exe = os.path.abspath(elaborator_exe)
        rdl_file = os.path.abspath(rdl_file)

        # Check prerequisites
        if not all(
            [
                self.check_executable(parser_exe),
                self.check_executable(elaborator_exe),
                self.check_rdl_file(rdl_file),
            ]
        ):
            return False

        test_name = Path(rdl_file).stem
        expect_failure = self.check_expect_elaboration_failure(rdl_file)

        if expect_failure:
            if self.verbose:
                print(f"[TEST] Testing validation for: {test_name} (expecting elaboration failure)")
        else:
            if self.verbose:
                print(f"[TEST] Testing JSON output for: {test_name}")

        # Create temporary directory
        with tempfile.TemporaryDirectory(prefix="json_test_") as temp_dir:
            temp_path = Path(temp_dir)

            # Test parser JSON output with custom filename
            if self.verbose:
                print("   [INFO] Testing parser JSON output...")
            ast_json = temp_path / f"{test_name}_ast.json"
            parser_cmd = [parser_exe, rdl_file, f"--ast={ast_json}"]

            if not self.run_command(parser_cmd):
                self.validator.log_error("Parser failed to generate JSON")
                return False

            if not ast_json.exists():
                self.validator.log_error("Parser JSON file not generated")
                return False

            self.validator.log_success(f"Parser JSON file generated: {ast_json}")

            # Validate parser JSON
            ast_data = self.validator.validate_json_file(str(ast_json))
            if not ast_data or not self.validator.validate_ast_json(ast_data):
                return False

            # Test elaborator - different handling for expected failure vs success
            if self.verbose:
                print("   [INFO] Testing elaborator JSON output...")
            elaborated_json = temp_path / f"{test_name}_elaborated.json"
            elaborator_cmd = [elaborator_exe, rdl_file, f"--ast={elaborated_json}"]

            elaborator_success = self.run_command(elaborator_cmd)

            if expect_failure:
                # For validation tests, we expect elaboration to fail
                if elaborator_success and elaborated_json.exists():
                    self.validator.log_error("Expected elaboration failure, but elaboration succeeded")
                    return False
                else:
                    self.validator.log_success("Elaboration failed as expected (validation test passed)")
                    # For expected failures, we can't test JSON generation, so we're done
                    if self.verbose:
                        print(f"[OK] Validation test completed successfully for: {test_name}")
                    return True
            else:
                # For normal tests, we expect elaboration to succeed
                if not elaborator_success:
                    self.validator.log_error("Elaborator failed to generate JSON")
                    return False

                if not elaborated_json.exists():
                    self.validator.log_error("Elaborator JSON file not generated")
                    return False

                self.validator.log_success(f"Elaborator JSON file generated: {elaborated_json}")

                # Validate elaborator JSON
                elaborated_data = self.validator.validate_json_file(str(elaborated_json))
                if not elaborated_data or not self.validator.validate_elaborated_json(elaborated_data):
                    return False

                # Test default filename generation for normal tests only
                if self.verbose:
                    print("  [VAL] Testing default filename generation...")

                # Test parser default filename
                parser_default_cmd = [parser_exe, rdl_file, "--ast"]
                if not self.run_command(parser_default_cmd, cwd=temp_dir):
                    self.validator.log_error("Parser failed with default AST filename")
                    return False

                default_ast = temp_path / f"{test_name}_ast.json"
                if default_ast.exists():
                    self.validator.log_success("Default AST filename generated correctly")
                else:
                    self.validator.log_error("Default AST filename not generated")
                    return False

                # Test elaborator default filename
                elaborator_default_cmd = [elaborator_exe, rdl_file, "--ast"]
                if not self.run_command(elaborator_default_cmd, cwd=temp_dir):
                    self.validator.log_error("Elaborator failed with default AST filename")
                    return False

                default_elaborated = temp_path / f"{test_name}_ast_elaborated.json"
                if default_elaborated.exists():
                    self.validator.log_success("Default elaborated filename generated correctly")
                else:
                    self.validator.log_error("Default elaborated filename not generated")
                    return False

                # Check file sizes
                ast_size = self.get_file_size(str(ast_json))
                elaborated_size = self.get_file_size(str(elaborated_json))

                if ast_size > 100:
                    self.validator.log_success(f"AST JSON has reasonable size: {ast_size} bytes")
                else:
                    self.validator.log_warning(f"AST JSON seems too small: {ast_size} bytes")

                if elaborated_size > 50:
                    self.validator.log_success(f"Elaborated JSON has reasonable size: {elaborated_size} bytes")
                else:
                    self.validator.log_warning(f"Elaborated JSON seems too small: {elaborated_size} bytes")

                # Cross-validate content
                self.validator.validate_content_match(ast_data, elaborated_data, rdl_file)

        self.validator.log_success(f"JSON test completed successfully for: {test_name}")
        return True


def main():
    parser = argparse.ArgumentParser(description="Validate and test SystemRDL AST JSON output")

    # Validation mode arguments
    parser.add_argument("--ast", help="Path to AST JSON file")
    parser.add_argument("--elaborated", help="Path to elaborated model JSON file")
    parser.add_argument("--rdl", help="Path to original RDL file (for context)")

    # End-to-end test mode arguments
    parser.add_argument("--test", action="store_true", help="Run end-to-end JSON test")
    parser.add_argument("--parser", help="Path to systemrdl_parser executable")
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
        # If it's already absolute, keep it as is
        if path.is_absolute():
            return str(path)
        # If it starts with ../ it's probably already relative to script dir
        if str(path).startswith("../"):
            return str(script_dir / path)
        # Otherwise, make it relative to project root
        return str(project_root / path)

    args.parser = adjust_path(args.parser)
    args.elaborator = adjust_path(args.elaborator)
    args.rdl = adjust_path(args.rdl)
    args.ast = adjust_path(args.ast)
    args.elaborated = adjust_path(args.elaborated)

    # Determine mode
    if args.test:
        # End-to-end test mode
        if not all([args.parser, args.elaborator, args.rdl]):
            print("[ERROR]: Test mode requires --parser, --elaborator, and --rdl arguments")
            # Provide usage examples
            print("Example usage:")
            print(
                "  python script/ast_output_validator.py --test "
                "--parser build/systemrdl_parser "
                "--elaborator build/systemrdl_elaborator "
                "--rdl test/test_minimal.rdl"
            )
            print(
                "  python ast_output_validator.py --test "
                "--parser ../build/systemrdl_parser "
                "--elaborator ../build/systemrdl_elaborator "
                "--rdl ../test/test_minimal.rdl"
            )
            sys.exit(1)

        tester = JsonTester(verbose=not args.quiet)
        success = tester.run_end_to_end_test(args.parser, args.elaborator, args.rdl)

        if not success or tester.validator.errors:
            print("\n[FAIL] Test FAILED")
            sys.exit(1)
        elif tester.validator.warnings and args.strict:
            print("\n[WARNING]  Test FAILED (strict mode, warnings treated as errors)")
            sys.exit(1)
        else:
            print("\n[OK] Test PASSED")
            sys.exit(0)
    else:
        # Validation mode
        if not args.ast and not args.elaborated:
            print("[ERROR]: Must specify at least one of --ast or --elaborated, or use --test mode")
            sys.exit(1)

        validator = JsonValidator(verbose=not args.quiet)
        ast_data = None
        elaborated_data = None

        # Validate AST JSON
        if args.ast:
            if not args.quiet:
                print(f"[INFO] Validating AST JSON: {args.ast}")
            ast_data = validator.validate_json_file(args.ast)
            if ast_data:
                validator.validate_ast_json(ast_data)

        # Validate elaborated JSON
        if args.elaborated:
            if not args.quiet:
                print(f"[INFO] Validating elaborated JSON: {args.elaborated}")
            elaborated_data = validator.validate_json_file(args.elaborated)
            if elaborated_data:
                validator.validate_elaborated_json(elaborated_data)

        # Cross-validate if both files are provided
        if ast_data and elaborated_data and args.rdl:
            if not args.quiet:
                print("[INFO] Cross-validating content consistency")
            validator.validate_content_match(ast_data, elaborated_data, args.rdl)

        # Summary
        if not args.quiet:
            print("\n[SUMMARY] Validation Summary:")
            print(f"   Errors: {len(validator.errors)}")
            print(f"   Warnings: {len(validator.warnings)}")

        if validator.errors:
            print("\n[FAIL] Validation FAILED")
            sys.exit(1)
        elif validator.warnings and args.strict:
            print("\n[WARNING]  Validation FAILED (strict mode, warnings treated as errors)")
            sys.exit(1)
        else:
            print("\n[OK] Validation PASSED")
            sys.exit(0)


if __name__ == "__main__":
    main()
