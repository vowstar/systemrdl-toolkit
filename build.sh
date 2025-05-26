#!/bin/bash

# Parse command line arguments
RUN_PYTHON_TESTS=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --python|-p)
            RUN_PYTHON_TESTS=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --python, -p    Run Python standard tests in addition to C++ tests"
            echo "  --help, -h      Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Create build directory
mkdir -p build
cd build

# Run CMake configuration
echo "Configuring with CMake..."
cmake ..

# Compile project
echo "Building project..."
make -j$(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Executables: ./systemrdl_parser, ./systemrdl_elaborator"
    echo "Usage: ./systemrdl_parser <input_file.rdl>"
    echo "       ./systemrdl_elaborator <input_file.rdl>"
    echo ""

    # Test all RDL files
    echo "🧪 Testing all RDL files with C++ tools..."
    echo "================================================"

    parser_passed=0
    elaborator_passed=0
    test_total=0

    for rdl_file in ../test/*.rdl; do
        if [ -f "$rdl_file" ]; then
            test_total=$((test_total + 1))
            echo ""
            echo "Testing $(basename "$rdl_file")..."
            echo "----------------------------------------"

            # Test systemrdl_parser
            echo "🔍 Testing with systemrdl_parser..."
            if ./systemrdl_parser "$rdl_file"; then
                echo "✅ Parser: $(basename "$rdl_file") - PASSED"
                parser_passed=$((parser_passed + 1))
            else
                echo "❌ Parser: $(basename "$rdl_file") - FAILED"
            fi

            # Test systemrdl_elaborator
            echo "🚀 Testing with systemrdl_elaborator..."
            if ./systemrdl_elaborator "$rdl_file"; then
                echo "✅ Elaborator: $(basename "$rdl_file") - PASSED"
                elaborator_passed=$((elaborator_passed + 1))
            else
                echo "❌ Elaborator: $(basename "$rdl_file") - FAILED"
            fi
        fi
    done

    echo ""
    echo "================================================"
    echo "🏁 C++ Tools Test Results:"
    echo "🔍 Parser Results:"
    echo "   ✅ Passed: $parser_passed/$test_total"
    echo "   ❌ Failed: $((test_total - parser_passed))/$test_total"
    echo "🚀 Elaborator Results:"
    echo "   ✅ Passed: $elaborator_passed/$test_total"
    echo "   ❌ Failed: $((test_total - elaborator_passed))/$test_total"

    # Run Python standard tests (only when specified parameter)
    if [ "$RUN_PYTHON_TESTS" = true ]; then
        echo ""
        echo "🐍 Running Python standard tests..."
        echo "================================================"

        cd ..
        if command -v python3 &> /dev/null; then
            if python3 -c "import systemrdl" 2>/dev/null; then
                echo "✅ systemrdl-compiler is available"
                python3 script/test_elaboration.py
                python_exit_code=$?

                if [ $python_exit_code -eq 0 ]; then
                    echo "✅ Python tests PASSED"
                else
                    echo "❌ Python tests FAILED"
                fi
            else
                echo "⚠️  systemrdl-compiler not installed. Cannot run Python tests."
                echo "   To install: pip install systemrdl-compiler"
            fi
        else
            echo "⚠️  Python3 not found. Cannot run Python tests."
        fi
    else
        echo ""
        echo "ℹ️  Python tests skipped (use --python or -p to run them)"
    fi

    echo ""
    echo "🎯 All tests completed!"

else
    echo "Build failed!"
    exit 1
fi