#!/usr/bin/env python3
"""
Compare SystemRDL implementations: Python (systemrdl-compiler) vs C++ (systemrdl_elaborator)
"""

import os
import sys
import glob
import subprocess
import tempfile
from pathlib import Path

class ImplementationComparator:
    def __init__(self, test_dir="test"):
        self.test_dir = test_dir
        self.cpp_exe = "build/systemrdl_elaborator"
        self.python_script = "script/rdl_semantic_validator.py"
        self.results = {
            'cpp_only_pass': [],
            'python_only_pass': [],
            'both_pass': [],
            'both_fail': [],
            'different_errors': [],
            'cpp_fail_python_pass': [],
            'python_fail_cpp_pass': []
        }

    def check_expect_elaboration_failure(self, rdl_file):
        """Check if RDL file is marked as expecting elaboration failure"""
        try:
            with open(rdl_file, 'r', encoding='utf-8') as f:
                # Check first few lines for EXPECT_ELABORATION_FAILURE marker
                for i, line in enumerate(f):
                    if i >= 10:  # Only check first 10 lines
                        break
                    if 'EXPECT_ELABORATION_FAILURE' in line:
                        return True
                return False
        except Exception:
            return False

    def run_cpp_implementation(self, rdl_file):
        """Run C++ implementation and return (success, output)"""
        try:
            result = subprocess.run(
                [self.cpp_exe, rdl_file],
                capture_output=True,
                text=True,
                timeout=10
            )
            return result.returncode == 0, result.stdout + result.stderr
        except Exception as e:
            return False, str(e)

    def run_python_implementation(self, rdl_file):
        """Run Python implementation and return (success, output)"""
        try:
            result = subprocess.run(
                ["python3", self.python_script, rdl_file],
                capture_output=True,
                text=True,
                timeout=10
            )
            return result.returncode == 0, result.stdout + result.stderr
        except Exception as e:
            return False, str(e)

    def extract_error_messages(self, output):
        """Extract key error messages from output"""
        errors = []
        lines = output.split('\n')
        for line in lines:
            if ('error:' in line.lower() or 'fatal:' in line.lower() or
                'field overlap detected' in line.lower() or
                'field exceeds' in line.lower() or
                'overlaps with' in line.lower()):
                # Clean up the error message
                error = line.strip()
                # For C++ format, remove "Line X:Y - " prefix
                if ' - ' in error and 'Line ' in error:
                    error = error.split(' - ', 1)[1]
                # For Python format, extract after file:line:col
                elif ':' in error:
                    parts = error.split(':', 3)
                    if len(parts) >= 4:
                        error = parts[3].strip()
                errors.append(error)
        return errors

    def compare_file(self, rdl_file):
        """Compare results for a single file"""
        file_name = os.path.basename(rdl_file)
        expect_failure = self.check_expect_elaboration_failure(rdl_file)

        print(f"\n📁 Testing: {file_name}")
        if expect_failure:
            print("   🎯 Expected: FAILURE (validation test)")
        else:
            print("   🎯 Expected: SUCCESS")

        # Run both implementations
        cpp_success, cpp_output = self.run_cpp_implementation(rdl_file)
        python_success, python_output = self.run_python_implementation(rdl_file)

        print(f"   🔧 C++ Result: {'✅ PASS' if cpp_success else '❌ FAIL'}")
        print(f"   🐍 Python Result: {'✅ PASS' if python_success else '❌ FAIL'}")

        # For expected failures, invert the logic
        if expect_failure:
            cpp_success = not cpp_success
            python_success = not python_success
            print(f"   🎯 C++ Validation: {'✅ PASS' if cpp_success else '❌ FAIL'}")
            print(f"   🎯 Python Validation: {'✅ PASS' if python_success else '❌ FAIL'}")

        # Categorize results
        if cpp_success and python_success:
            self.results['both_pass'].append(file_name)
            print("   📊 Status: BOTH PASS ✅")
        elif not cpp_success and not python_success:
            # Check if error messages are similar
            cpp_errors = self.extract_error_messages(cpp_output)
            python_errors = self.extract_error_messages(python_output)

            if self.errors_similar(cpp_errors, python_errors):
                self.results['both_fail'].append(file_name)
                print("   📊 Status: BOTH FAIL (similar errors) ⚠️")
            else:
                self.results['different_errors'].append((file_name, cpp_errors, python_errors))
                print("   📊 Status: BOTH FAIL (different errors) ⚠️")
                print(f"      C++ errors: {cpp_errors}")
                print(f"      Python errors: {python_errors}")
        elif cpp_success and not python_success:
            self.results['cpp_only_pass'].append((file_name, python_output))
            print("   📊 Status: C++ PASS, Python FAIL ⚠️")
            print(f"      Python error: {self.extract_error_messages(python_output)}")
        elif not cpp_success and python_success:
            self.results['python_only_pass'].append((file_name, cpp_output))
            print("   📊 Status: Python PASS, C++ FAIL ⚠️")
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
            if 'overlap' in error.lower():
                cpp_concepts.add('overlap')
            if 'exceed' in error.lower() or 'boundary' in error.lower():
                cpp_concepts.add('boundary')
            if 'power of 2' in error.lower():
                cpp_concepts.add('power_of_2')

        for error in python_errors:
            if 'overlap' in error.lower():
                python_concepts.add('overlap')
            if 'exceed' in error.lower() or 'boundary' in error.lower():
                python_concepts.add('boundary')
            if 'power of 2' in error.lower():
                python_concepts.add('power_of_2')

        return len(cpp_concepts.intersection(python_concepts)) > 0

    def run_comparison(self):
        """Run comparison on all RDL files"""
        if not os.path.exists(self.test_dir):
            print(f"❌ Test directory does not exist: {self.test_dir}")
            return False

        rdl_files = glob.glob(os.path.join(self.test_dir, "*.rdl"))
        if not rdl_files:
            print(f"❌ No RDL files found in directory {self.test_dir}")
            return False

        print(f"🎯 Found {len(rdl_files)} RDL files for comparison")
        print("="*80)

        # Test executables
        if not os.path.exists(self.cpp_exe):
            print(f"❌ C++ executable not found: {self.cpp_exe}")
            return False

        if not os.path.exists(self.python_script):
            print(f"❌ Python script not found: {self.python_script}")
            return False

        for rdl_file in sorted(rdl_files):
            self.compare_file(rdl_file)

        self.print_summary()
        return True

    def print_summary(self):
        """Print comparison summary"""
        print("\n" + "="*80)
        print("📊 COMPARISON SUMMARY")
        print("="*80)

        total_files = (len(self.results['both_pass']) +
                      len(self.results['both_fail']) +
                      len(self.results['different_errors']) +
                      len(self.results['cpp_only_pass']) +
                      len(self.results['python_only_pass']))

        print(f"📁 Total files tested: {total_files}")
        print(f"✅ Both implementations pass: {len(self.results['both_pass'])}")
        print(f"⚠️  Both implementations fail (similar): {len(self.results['both_fail'])}")
        print(f"⚠️  Both implementations fail (different): {len(self.results['different_errors'])}")
        print(f"🔧 C++ only passes: {len(self.results['cpp_only_pass'])}")
        print(f"🐍 Python only passes: {len(self.results['python_only_pass'])}")

        # Detailed breakdown
        if self.results['both_pass']:
            print(f"\n✅ BOTH PASS ({len(self.results['both_pass'])}):")
            for file_name in self.results['both_pass']:
                print(f"   - {file_name}")

        if self.results['cpp_only_pass']:
            print(f"\n🔧 C++ ONLY PASS ({len(self.results['cpp_only_pass'])}):")
            for file_name, python_error in self.results['cpp_only_pass']:
                print(f"   - {file_name}")
                errors = self.extract_error_messages(python_error)
                if errors:
                    print(f"     Python error: {errors[0]}")

        if self.results['python_only_pass']:
            print(f"\n🐍 PYTHON ONLY PASS ({len(self.results['python_only_pass'])}):")
            for file_name, cpp_error in self.results['python_only_pass']:
                print(f"   - {file_name}")
                errors = self.extract_error_messages(cpp_error)
                if errors:
                    print(f"     C++ error: {errors[0]}")

        if self.results['different_errors']:
            print(f"\n⚠️  DIFFERENT ERROR TYPES ({len(self.results['different_errors'])}):")
            for file_name, cpp_errors, python_errors in self.results['different_errors']:
                print(f"   - {file_name}")
                print(f"     C++: {cpp_errors}")
                print(f"     Python: {python_errors}")

        # Analysis
        print(f"\n🔍 ANALYSIS:")
        compatibility = len(self.results['both_pass']) + len(self.results['both_fail'])
        compatibility_percent = (compatibility / total_files) * 100 if total_files > 0 else 0

        print(f"   📈 Compatibility: {compatibility}/{total_files} ({compatibility_percent:.1f}%)")

        if len(self.results['cpp_only_pass']) > 0:
            print(f"   🔧 C++ implementation may be more permissive")
        if len(self.results['python_only_pass']) > 0:
            print(f"   🐍 Python implementation may be more permissive")
        if len(self.results['different_errors']) > 0:
            print(f"   ⚠️  Error message differences detected")

def main():
    if len(sys.argv) > 1:
        test_dir = sys.argv[1]
    else:
        test_dir = "test"

    comparator = ImplementationComparator(test_dir)
    success = comparator.run_comparison()
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()