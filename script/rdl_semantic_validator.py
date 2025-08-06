#!/usr/bin/env python3
"""
Sample script demonstrating SystemRDL elaboration process
Requires installation: pip install systemrdl-compiler
"""

import glob
import os
import sys

from systemrdl import RDLCompileError, RDLCompiler


def check_expect_elaboration_failure(rdl_file):
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


def demonstrate_elaboration(rdl_file):
    """Demonstrate SystemRDL elaboration process"""

    expect_failure = check_expect_elaboration_failure(rdl_file)

    # Create compiler instance
    rdlc = RDLCompiler()

    try:
        # Compile SystemRDL file
        print(f"[CPP] Compiling SystemRDL file: {rdl_file}")
        rdlc.compile_file(rdl_file)

        # Elaborate - this is the key step!
        if expect_failure:
            print("[START] Starting elaboration (expecting failure for validation test)...")
        else:
            print("[START] Starting elaboration...")
        root = rdlc.elaborate()

        # If we get here and expected failure, that's wrong
        if expect_failure:
            print("[FAIL] Expected elaboration failure, but elaboration succeeded")
            return False
        else:
            print("[OK] Elaboration successful!")
            print("\n" + "=" * 50)
            print(f"[SUMMARY] Elaborated register model information ({rdl_file}):")
            print("=" * 50)

            # Traverse elaborated model
            traverse_node(root, 0)
            return True

    except RDLCompileError as e:
        if expect_failure:
            print("[OK] Elaboration failed as expected (validation test passed)")
            return True
        else:
            print(f"[FAIL] Compilation error ({rdl_file}): {e}")
            return False
    except Exception as e:
        if expect_failure:
            print("[OK] Elaboration failed as expected (validation test passed)")
            return True
        else:
            print(f"[FAIL] Other error ({rdl_file}): {e}")
            return False


def traverse_node(node, depth=0):
    """Recursively traverse elaborated nodes"""
    indent = "  " * depth

    # Print node information
    print(f"{indent}[NODE] {node.__class__.__name__}: {node.inst_name}")

    # Special handling for FieldNode - show detailed field information
    from systemrdl.node import FieldNode

    if isinstance(node, FieldNode):
        try:
            # Show bit range information
            msb = node.msb
            lsb = node.lsb
            width = node.width
            print(f"{indent}   [VAL] Bit range: [{msb}:{lsb}] (width: {width})")
        except (ValueError, AttributeError):
            print(f"{indent}   [VAL] Bit range: <cannot determine>")

        # Show field properties
        try:
            # Software access
            sw = node.get_property("sw")
            hw = node.get_property("hw")
            if sw or hw:
                print(f"{indent}   [CPP] Access: sw={sw}, hw={hw}")
        except LookupError:
            pass

        try:
            # Reset value
            reset = node.get_property("reset")
            if reset is not None:
                print(f"{indent}   [RESET] Reset value: 0x{reset:X}")
        except (LookupError, TypeError):
            pass

        try:
            # Field width (if explicitly set)
            fieldwidth = node.get_property("fieldwidth")
            if fieldwidth is not None:
                print(f"{indent}   [SIZE] Field width: {fieldwidth}")
        except LookupError:
            pass

    # Safely get address information - only try to get address for addressable nodes
    from systemrdl.node import AddressableNode

    if isinstance(node, AddressableNode):
        try:
            addr = node.absolute_address
            print(f"{indent}   [ADDR] Address: 0x{addr:08X}")
        except ValueError:
            # Cannot calculate address when array index is unknown
            print(f"{indent}   [ADDR] Address: <array index unknown>")

        try:
            size = node.size
            print(f"{indent}   [SIZE] Size: {size} bytes")
        except ValueError:
            print(f"{indent}   [SIZE] Size: <cannot determine>")

        # Check if it's an array
        if hasattr(node, "array_dimensions") and node.array_dimensions:
            print(f"{indent}   [ARRAY] Array dimensions: {node.array_dimensions}")
            try:
                stride = node.array_stride
                print(f"{indent}   [ARRAY] Array stride: {stride}")
            except ValueError:
                print(f"{indent}   [ARRAY] Array stride: <cannot determine>")

    # If there's a description, show description
    if hasattr(node, "get_property"):
        try:
            desc = node.get_property("desc")
            if desc:  # Only show when description is not empty
                if len(desc) > 50:
                    desc = desc[:47] + "..."
                print(f"{indent}   [DESC] Description: {desc}")
        except LookupError:
            # Some node types may not support desc attribute
            pass

    # Recursively process child nodes
    if hasattr(node, "children"):
        for child in node.children():
            traverse_node(child, depth + 1)


def test_all_rdl_files(test_dir="test"):
    """Test all RDL files in specified directory"""
    if not os.path.exists(test_dir):
        print(f"[FAIL] Test directory does not exist: {test_dir}")
        return False

    rdl_files = glob.glob(os.path.join(test_dir, "*.rdl"))
    if not rdl_files:
        print(f"[FAIL] No RDL files found in directory {test_dir}")
        return False

    print(f"[VAL] Found {len(rdl_files)} RDL files for testing")
    print("=" * 60)

    success_count = 0
    total_count = len(rdl_files)

    for rdl_file in sorted(rdl_files):
        print(f"\n{'='*60}")
        print(f"Testing file {os.path.basename(rdl_file)} ({success_count + 1}/{total_count})")
        print(f"{'='*60}")

        if demonstrate_elaboration(rdl_file):
            success_count += 1

        print(f"\n{'='*60}")

    print("\n[EXIT] Testing complete!")
    print(f"[OK] Success: {success_count}/{total_count}")
    print(f"[FAIL] Failed: {total_count - success_count}/{total_count}")

    return success_count == total_count


if __name__ == "__main__":
    if len(sys.argv) > 1:
        # If command line argument provided, test specified file
        rdl_file = sys.argv[1]
        if os.path.exists(rdl_file):
            success = demonstrate_elaboration(rdl_file)
            sys.exit(0 if success else 1)
        else:
            print(f"[FAIL] File does not exist: {rdl_file}")
            sys.exit(1)
    else:
        # If no argument provided, test all files in test directory
        success = test_all_rdl_files()
        sys.exit(0 if success else 1)
