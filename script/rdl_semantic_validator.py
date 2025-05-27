#!/usr/bin/env python3
"""
Sample script demonstrating SystemRDL elaboration process
Requires installation: pip install systemrdl-compiler
"""

import sys
import os
import glob
from systemrdl import RDLCompiler, RDLCompileError

def demonstrate_elaboration(rdl_file):
    """Demonstrate SystemRDL elaboration process"""

    # Create compiler instance
    rdlc = RDLCompiler()

    try:
        # Compile SystemRDL file
        print(f"🔧 Compiling SystemRDL file: {rdl_file}")
        rdlc.compile_file(rdl_file)

        # Elaborate - this is the key step!
        print("🚀 Starting elaboration...")
        root = rdlc.elaborate()

        print("✅ Elaboration successful!")
        print("\n" + "="*50)
        print(f"📊 Elaborated register model information ({rdl_file}):")
        print("="*50)

        # Traverse elaborated model
        traverse_node(root, 0)
        return True

    except RDLCompileError as e:
        print(f"❌ Compilation error ({rdl_file}): {e}")
        return False
    except Exception as e:
        print(f"❌ Other error ({rdl_file}): {e}")
        return False

def traverse_node(node, depth=0):
    """Recursively traverse elaborated nodes"""
    indent = "  " * depth

    # Print node information
    print(f"{indent}📦 {node.__class__.__name__}: {node.inst_name}")

    # Safely get address information - only try to get address for addressable nodes
    from systemrdl.node import AddressableNode
    if isinstance(node, AddressableNode):
        try:
            addr = node.absolute_address
            print(f"{indent}   📍 Address: 0x{addr:08X}")
        except ValueError:
            # Cannot calculate address when array index is unknown
            print(f"{indent}   📍 Address: <array index unknown>")

        try:
            size = node.size
            print(f"{indent}   📏 Size: {size} bytes")
        except ValueError:
            print(f"{indent}   📏 Size: <cannot determine>")

        # Check if it's an array
        if hasattr(node, 'array_dimensions') and node.array_dimensions:
            print(f"{indent}   🔢 Array dimensions: {node.array_dimensions}")
            try:
                stride = node.array_stride
                print(f"{indent}   🔢 Array stride: {stride}")
            except ValueError:
                print(f"{indent}   🔢 Array stride: <cannot determine>")

    # If there's a description, show description
    if hasattr(node, 'get_property'):
        try:
            desc = node.get_property('desc')
            if desc:  # Only show when description is not empty
                if len(desc) > 50:
                    desc = desc[:47] + "..."
                print(f"{indent}   📝 Description: {desc}")
        except LookupError:
            # Some node types may not support desc attribute
            pass

    # Recursively process child nodes
    if hasattr(node, 'children'):
        for child in node.children():
            traverse_node(child, depth + 1)

def test_all_rdl_files(test_dir="test"):
    """Test all RDL files in specified directory"""
    if not os.path.exists(test_dir):
        print(f"❌ Test directory does not exist: {test_dir}")
        return False

    rdl_files = glob.glob(os.path.join(test_dir, "*.rdl"))
    if not rdl_files:
        print(f"❌ No RDL files found in directory {test_dir}")
        return False

    print(f"🎯 Found {len(rdl_files)} RDL files for testing")
    print("="*60)

    success_count = 0
    total_count = len(rdl_files)

    for rdl_file in sorted(rdl_files):
        print(f"\n{'='*60}")
        print(f"Testing file {os.path.basename(rdl_file)} ({success_count + 1}/{total_count})")
        print(f"{'='*60}")

        if demonstrate_elaboration(rdl_file):
            success_count += 1

        print(f"\n{'='*60}")

    print(f"\n🏁 Testing complete!")
    print(f"✅ Success: {success_count}/{total_count}")
    print(f"❌ Failed: {total_count - success_count}/{total_count}")

    return success_count == total_count

if __name__ == "__main__":
    if len(sys.argv) > 1:
        # If command line argument provided, test specified file
        rdl_file = sys.argv[1]
        if os.path.exists(rdl_file):
            success = demonstrate_elaboration(rdl_file)
            sys.exit(0 if success else 1)
        else:
            print(f"❌ File does not exist: {rdl_file}")
            sys.exit(1)
    else:
        # If no argument provided, test all files in test directory
        success = test_all_rdl_files()
        sys.exit(0 if success else 1)
