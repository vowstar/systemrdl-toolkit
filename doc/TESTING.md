# Testing and Validation

The project includes testing capabilities with both C++ tools and Python validation scripts:

## Setup Requirements

Ensure Python virtual environment is set up and activated (see [Python Dependencies](#python-dependencies) section for
detailed setup):

```bash
# If not already set up, create and install dependencies
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

# If already set up, just activate
source .venv/bin/activate
```

## Quick Testing

```bash
# Run fast tests (JSON + semantic validation)
cd build
make test-fast

# Run JSON output tests only
make test-json

# Run semantic validation tests only
make test-semantic
```

## Python Validation Scripts

The project includes two main Python scripts for validation:

### 1. RDL Semantic Validator (`script/rdl_semantic_validator.py`)

This script validates SystemRDL files using the official SystemRDL compiler and demonstrates the elaboration process:

```bash
# Validate specific RDL file
python3 script/rdl_semantic_validator.py test/test_minimal.rdl

# Validate all RDL files in test directory
python3 script/rdl_semantic_validator.py

# The script will show:
# - Compilation status
# - Elaboration results
# - Node hierarchy with addresses and properties
# - Array information and descriptions
```

**Features:**

- Uses official `systemrdl-compiler` for validation
- Provides detailed elaboration output with addresses and sizes
- Shows node hierarchy and properties
- Validates array dimensions and strides
- Displays property descriptions where available

### 2. JSON Output Validator (`script/json_output_validator.py`)

This script validates and tests JSON output from the C++ tools:

```bash
# Run end-to-end test (generate and validate JSON)
python3 script/json_output_validator.py --test \
    --parser build/systemrdl_parser \
    --elaborator build/systemrdl_elaborator \
    --rdl test/test_minimal.rdl

# Validate existing JSON files
python3 script/json_output_validator.py --ast output_ast.json
python3 script/json_output_validator.py --elaborated output_elaborated.json

# Validate both with original RDL file for context
python3 script/json_output_validator.py --ast ast.json --elaborated elaborated.json --rdl input.rdl

# Strict mode (treat warnings as errors)
python3 script/json_output_validator.py --ast output.json --strict

# Quiet mode (show only errors)
python3 script/json_output_validator.py --test --parser build/systemrdl_parser --elaborator build/systemrdl_elaborator --rdl test/test_minimal.rdl --quiet
```

**Features:**

- Validates JSON schema and structure
- Checks AST JSON format compliance
- Validates elaborated model format
- Performs end-to-end testing
- Compares consistency between parser and elaborator outputs
- Supports individual file validation and batch testing

- `script/csv2rdl_validator.py` - CSV to SystemRDL converter validation suite
  - Three-tier validation: conversion success, syntax validation, content validation
  - Auto-discovers CSV test files using `test_csv_*.csv` naming convention
  - Cross-directory execution with automatic project path detection
  - Comprehensive test coverage: basic, multiline, delimiters, fuzzy matching
  - Professional validation framework with detailed reporting and exit codes

## Testing commands

```bash
# Run all tests (parser + elaborator + JSON + semantic)
make test-all

# Standard CTest execution
make test

# Verbose output with details
ctest --output-on-failure --verbose

# Custom target for testing
make run-tests
```

### Test Categories

```bash
# Test only the parser
make test-parser

# Test only the elaborator
make test-elaborator

# Or using CTest labels
ctest -L parser --output-on-failure
ctest -L elaborator --output-on-failure
ctest -L json --output-on-failure
ctest -L semantic --output-on-failure
```

### Individual Test Execution

```bash
# Test specific file with parser
ctest -R "parser_test_minimal" --output-on-failure

# Test specific file with elaborator
ctest -R "elaborator_test_minimal" --output-on-failure

# Test specific JSON output
ctest -R "json_test_minimal" --output-on-failure

# Run semantic validation for specific file
ctest -R "rdl_semantic_validation" --output-on-failure
```

### Available Test Targets

- `test-fast` - Quick tests (JSON + semantic validation) for rapid development
- `test-json` - JSON output validation tests using Python validator
- `test-semantic` - RDL semantic validation using Python SystemRDL compiler
- `test-parser` - SystemRDL parser tests
- `test-elaborator` - SystemRDL elaborator tests
- `test-all` - Complete test suite

### Test Files

The project includes 16 test files covering various SystemRDL features:

- `test_minimal.rdl` - Basic SystemRDL structure
- `test_basic_chip.rdl` - Simple chip layout
- `test_bit_ranges.rdl` - Field bit range specifications
- `test_complex_arrays.rdl` - Multi-dimensional arrays
- `test_component_reuse.rdl` - Component definition reuse
- `test_enum_struct.rdl` - Enumerations and structures
- `test_expressions.rdl` - SystemRDL expressions
- `test_field_properties.rdl` - Field property assignments
- `test_memory.rdl` - Memory component definitions
- `test_param_arrays.rdl` - Parameterized arrays
- `test_param_expressions.rdl` - Parameter expressions
- `test_parameterized.rdl` - Parameterized components
- `test_parameters.rdl` - Parameter definitions
- `test_regfile_array.rdl` - Register file arrays
- `test_simple_enum.rdl` - Basic enumerations
- `test_simple_param_ref.rdl` - Parameter references
- `test_auto_reserved_fields.rdl` - Automatic reserved field generation for register gaps
- `test_comprehensive_gaps.rdl` - Comprehensive gap detection scenarios and edge cases
- `test_field_validation_comprehensive.rdl` - Comprehensive field validation test suite (overlaps, boundaries, mixed scenarios)
- `test_field_overlap.rdl` - Field overlap detection test cases
- `test_field_boundary.rdl` - Field boundary validation test cases
- `test_address_overlap.rdl` - Register address overlap detection tests
