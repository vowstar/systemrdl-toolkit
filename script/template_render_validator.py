#!/usr/bin/env python3
"""
SystemRDL Template Rendering Validator
Tests the systemrdl_render tool with various templates and RDL files
"""

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def find_tool_executable(tool_name):
    """Find the tool executable in build directory or PATH"""
    # First try build directory
    for build_dir in ["build", "build-release", "build-debug"]:
        tool_path = Path(build_dir) / tool_name
        if tool_path.exists() and tool_path.is_file():
            return str(tool_path)

    # Try relative to script directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    for build_dir in ["build", "build-release", "build-debug"]:
        tool_path = project_root / build_dir / tool_name
        if tool_path.exists() and tool_path.is_file():
            return str(tool_path)

    # Try system PATH
    try:
        result = subprocess.run(["which", tool_name], capture_output=True, text=True)
        if result.returncode == 0:
            return result.stdout.strip()
    except FileNotFoundError:
        pass

    return None


def run_template_render(rdl_file, template_file, output_file=None, verbose=False, use_ast=False):
    """Run systemrdl_render tool with specified parameters"""
    tool_path = find_tool_executable("systemrdl_render")
    if not tool_path:
        print("[FAIL] systemrdl_render tool not found")
        return False, ""

    cmd = [tool_path, rdl_file, "-t", template_file]
    if output_file:
        cmd.extend(["-o", output_file])
    if use_ast:
        cmd.append("--ast")  # Use full AST JSON instead of simplified (default)
    if verbose:
        cmd.append("--verbose")

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        return result.returncode == 0, result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return False, "Template rendering timed out"
    except Exception as e:
        return False, f"Failed to run template rendering: {e}"


def validate_output_content(content, template_name):
    """Validate output content based on expected characteristics"""
    if not content or len(content) < 10:
        return False, "Content too short"

    # Check for common SystemRDL template characteristics
    has_systemrdl_content = any(
        keyword in content.lower() for keyword in ["systemrdl", "register", "field", "address", "auto-generated"]
    )

    if not has_systemrdl_content:
        return False, "Missing SystemRDL-related content"

    # Template-specific validation based on template name patterns
    if "header" in template_name.lower():
        # C header file characteristics
        expected_features = [
            "#ifndef" in content or "#define" in content,  # Header guards or defines
            any(keyword in content for keyword in ["_OFFSET", "_ADDR", "_LSB", "_MSB", "_WIDTH"]),  # Register/field defines
            "/*" in content or "//" in content,  # C-style comments
        ]
        if not any(expected_features):
            return False, "Missing C header characteristics"

    elif "doc" in template_name.lower() or "md" in template_name.lower():
        # Markdown characteristics
        expected_features = [
            content.startswith("#") or "# " in content,  # Markdown headers
            "**" in content or "*" in content,  # Markdown emphasis
            "|" in content or "-" in content,  # Tables or horizontal rules
            "Register" in content and "Field" in content,  # Documentation content
        ]
        if not any(expected_features):
            return False, "Missing Markdown characteristics"

    elif "verilog" in template_name.lower() or template_name.endswith(".v"):
        # Verilog RTL characteristics
        expected_features = [
            "module " in content,  # Module declaration
            "endmodule" in content,  # Module end
            any(keyword in content for keyword in ["input", "output", "wire", "reg"]),  # Port/signal declarations
            any(keyword in content for keyword in ["always", "assign", "case"]),  # RTL constructs
            "clk" in content and "rst" in content,  # Clock and reset
        ]
        if not any(expected_features):
            return False, "Missing Verilog RTL characteristics"

    return True, "Content validation passed"


def test_template_with_rdl(template_file, rdl_file, temp_dir):
    """Test a specific template with a specific RDL file"""
    template_name = Path(template_file).stem
    rdl_name = Path(rdl_file).stem

    # Extract output filename from template name, removing prefix
    # Examples: test_j2_ast_header.h -> header.h, test_j2_json_header.h -> header.h
    if "_j2_" in template_name:
        # Split by _j2_ and take the part after
        after_j2 = template_name.split("_j2_", 1)[1]
        # Remove ast_ or json_ prefix if present
        if after_j2.startswith("ast_"):
            output_suffix = after_j2[4:]  # Remove "ast_"
        elif after_j2.startswith("json_"):
            output_suffix = after_j2[5:]  # Remove "json_"
        else:
            output_suffix = after_j2
        output_file = os.path.join(temp_dir, f"{rdl_name}_{output_suffix}")
    else:
        output_file = os.path.join(temp_dir, f"{rdl_name}_{template_name}.txt")

    print(f"  [CPP] Testing {template_name} with {rdl_name}")

    # Run template rendering - determine format based on template name prefix
    # Templates with "ast" prefix expect full AST JSON format
    # Templates with "json" prefix expect simplified JSON format (default)
    use_ast = "_ast_" in template_name.lower()
    success, output = run_template_render(rdl_file, template_file, output_file, use_ast=use_ast)

    if not success:
        print(f"    [FAIL] Template rendering failed: {output}")
        return False

    # Validate output exists and has content
    if not os.path.exists(output_file):
        print(f"    [FAIL] Output file not created: {output_file}")
        return False

    try:
        with open(output_file, "r", encoding="utf-8") as f:
            content = f.read()

        # Validate content based on characteristics
        is_valid, validation_msg = validate_output_content(content, template_name)
        if not is_valid:
            print(f"    [FAIL] Content validation failed: {validation_msg}")
            print(f"    [FILE] Content preview: {content[:200]}...")
            return False

        print(f"    [OK] Success: {validation_msg} ({len(content)} chars)")
        return True
    except Exception as e:
        print(f"    [FAIL] Failed to read output: {e}")
        return False


def test_template_rendering():
    """Test template rendering with various combinations"""
    print("[START] SystemRDL Template Rendering Validator")
    print("=" * 60)

    # Find test files
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    test_dir = project_root / "test"

    # Find RDL files
    rdl_files = list(test_dir.glob("*.rdl"))
    if not rdl_files:
        print("[FAIL] No RDL test files found")
        return False

    # Find template files
    template_files = list(test_dir.glob("test_j2_*.j2"))
    if not template_files:
        print("[FAIL] No template files found")
        return False

    # Check if tool exists
    tool_path = find_tool_executable("systemrdl_render")
    if not tool_path:
        print("[FAIL] systemrdl_render tool not found. Please build the project first.")
        return False

    print(f"[CPP] Found tool: {tool_path}")
    print(f"[FOLDER] Found {len(rdl_files)} RDL files")
    print(f"[FILE] Found {len(template_files)} template files")
    print()

    success_count = 0
    total_tests = 0

    # Create temporary directory for outputs
    with tempfile.TemporaryDirectory() as temp_dir:
        print(f"[TEMP] Using temporary directory: {temp_dir}")
        print()

        # Test each template with a representative RDL file
        test_rdl_files = [
            rdl_files[0],  # First RDL file
        ]

        # If we have test_basic_chip.rdl, use that as it's simple
        for rdl_file in rdl_files:
            if "basic_chip" in rdl_file.name:
                test_rdl_files = [rdl_file]
                break

        for template_file in sorted(template_files):
            template_name = template_file.stem
            print(f"[LIST] Testing template: {template_name}")

            template_success = 0
            template_total = 0

            for rdl_file in test_rdl_files:
                total_tests += 1
                template_total += 1

                if test_template_with_rdl(str(template_file), str(rdl_file), temp_dir):
                    success_count += 1
                    template_success += 1

            print(f"  [SUMMARY] Template results: {template_success}/{template_total}")
            print()

    # Test auto-generated filename functionality
    print("[CPP] Testing auto-generated filenames...")
    with tempfile.TemporaryDirectory() as temp_dir:
        # Change to temp directory to test auto-generated names
        original_cwd = os.getcwd()
        try:
            os.chdir(temp_dir)

            # Test with first template and RDL file
            if template_files and test_rdl_files:
                template_file = template_files[0]
                rdl_file = test_rdl_files[0]

                success, output = run_template_render(str(rdl_file), str(template_file), use_ast=True)
                if success:
                    print("  [OK] Auto-generated filename test passed")
                    success_count += 1
                else:
                    print(f"  [FAIL] Auto-generated filename test failed: {output}")
                total_tests += 1
        finally:
            os.chdir(original_cwd)

    print("=" * 60)
    print("[EXIT] Template Rendering Validation Complete!")
    print(f"[OK] Success: {success_count}/{total_tests}")
    print(f"[FAIL] Failed: {total_tests - success_count}/{total_tests}")

    if success_count == total_tests:
        print("[OK] All template rendering tests passed!")
        return True
    else:
        print("[FATAL] Some template rendering tests failed!")
        return False


def test_error_conditions():
    """Test error conditions and edge cases"""
    print("\n[ERROR] Testing Error Conditions")
    print("-" * 40)

    tool_path = find_tool_executable("systemrdl_render")
    if not tool_path:
        print("[FAIL] Tool not found, skipping error condition tests")
        return True

    error_tests_passed = 0
    total_error_tests = 0

    # Test 1: Missing template file
    total_error_tests += 1
    try:
        result = subprocess.run(
            [tool_path, "nonexistent.rdl", "-t", "nonexistent.j2"], capture_output=True, text=True, timeout=10
        )
        if result.returncode != 0:
            print("  [OK] Missing template file properly handled")
            error_tests_passed += 1
        else:
            print("  [FAIL] Missing template file should have failed")
    except Exception as e:
        print(f"  [FAIL] Error testing missing template: {e}")

    # Test 2: Missing RDL file
    total_error_tests += 1
    try:
        # Create a dummy template file
        with tempfile.NamedTemporaryFile(mode="w", suffix=".j2", delete=False) as tf:
            tf.write("{{ timestamp }}")
            temp_template = tf.name

        try:
            result = subprocess.run(
                [tool_path, "nonexistent.rdl", "-t", temp_template], capture_output=True, text=True, timeout=10
            )
            if result.returncode != 0:
                print("  [OK] Missing RDL file properly handled")
                error_tests_passed += 1
            else:
                print("  [FAIL] Missing RDL file should have failed")
        finally:
            os.unlink(temp_template)
    except Exception as e:
        print(f"  [FAIL] Error testing missing RDL file: {e}")

    print(f"[SUMMARY] Error condition tests: {error_tests_passed}/{total_error_tests}")
    return error_tests_passed == total_error_tests


def main():
    """Main entry point"""
    print("SystemRDL Template Rendering Validator")
    print("=" * 80)

    # Change to project root directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    os.chdir(project_root)

    success = True

    # Run main template rendering tests
    if not test_template_rendering():
        success = False

    # Run error condition tests
    if not test_error_conditions():
        success = False

    print("\n" + "=" * 80)
    if success:
        print("[OK] All template rendering validation tests passed!")
        sys.exit(0)
    else:
        print("[FATAL] Some template rendering validation tests failed!")
        sys.exit(1)


if __name__ == "__main__":
    main()
