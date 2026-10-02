# Testing and Validation

Tests come in two kinds: C++ unit tests built by CTest, and Python checks that
compare this toolkit against the standard and against the reference
implementation.

## Setup Requirements

Ensure the Python virtual environment is set up and activated, as described in
[Python Dependencies](BUILD.md#python-dependencies).

## Python Validation Scripts

The project includes these Python validation scripts:

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
- Compares consistency between parser and elaborator outputs
- Supports individual file validation and batch testing

### 3. CSV to SystemRDL Converter Validator (`script/csv2rdl_validator.py`)

- Three-tier validation: conversion success, syntax validation, content validation
- Auto-discovers CSV test files using `test_csv_*.csv` naming convention
- Cross-directory execution with automatic project path detection

### 4. Spec Conformance Checker (`script/spec_conformance_check.py`)

Checks the elaborator against the worked examples in the SystemRDL 2.0
standard. Each example lives in `test/test_spec_*.rdl` with the addresses and
bit positions the standard states recorded as `SPEC-EXPECT` comment lines.

This checker consults no other implementation. A second implementation can tell
you that two tools disagree; only the standard says which one is right.

Examples the toolkit does not satisfy are listed in `KNOWN_FAILURES` alongside
the clause they belong to. An entry that starts passing is reported as an error
so the list cannot go stale.

### 5. Value Comparison (`script/compare_implementations.py`)

Elaborates every RDL file with both this toolkit and the reference
implementation and compares the resulting register addresses, register widths,
field bit positions and reset values. Matching exit status proves nothing about
the numbers that reach RTL and firmware headers.

Files named `test_spec_*.rdl` are skipped here because the spec conformance
checker already covers them against the standard itself.

## Running Tests

```bash
make test                   # Standard CTest run
make test-all               # Full suite (parser + elaborator + JSON + semantic)
make run-tests              # Full suite with verbose output
make test-fast              # Fast group (AST, JSON, semantic, CSV2RDL, template, example)
make test-json              # JSON output tests
make test-semantic          # RDL semantic validation
make test-parser            # Parser tests
make test-elaborator        # Elaborator tests
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

### Adding a test

Everything under `test/` is discovered by name, so adding a file is enough. No
list needs updating, which is why this document does not keep one.

| Pattern | What it is |
| -- | -- |
| `test_*.rdl` | Elaborated and compared against the reference implementation |
| `test_*_fail.rdl` | Expected to fail elaboration, and to fail for its own stated reason |
| `test_spec_*.rdl` | A worked example from the standard, with `SPEC-EXPECT` lines giving the expected values |
| `test_csv_*.csv` | An RCSV case for the converter |
| `test_*.cpp` | A C++ unit test, built and run by CTest |

A `_fail` file should break exactly one rule. If it fails for two reasons, the
one you meant to test can rot away unnoticed while the file still passes.

Start the file with a comment saying what it covers and, for a `_fail` file,
`EXPECT_ELABORATION_FAILURE` with the reason.
