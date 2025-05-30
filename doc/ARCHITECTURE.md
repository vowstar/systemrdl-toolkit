
# File Description

## Core C++ Components

- `parser_main.cpp` - Main program for the SystemRDL parser with JSON export capability
- `elaborator_main.cpp` - Main program for the SystemRDL elaborator with JSON export capability
- `elaborator.cpp/.h` - Elaboration engine implementation for semantic analysis
- `cmdline_parser.h` - Command line argument parsing utilities
- `CMakeLists.txt` - CMake build configuration with integrated testing and ANTLR4 management

## CSV to SystemRDL Converter

- `csv2rdl_main.cpp` - CSV to SystemRDL converter with header matching and multi-line support
- `script/csv2rdl_validator.py` - Comprehensive validation suite for CSV converter testing
- `test/test_csv_*.csv` - CSV test files covering various scenarios (basic, multiline, delimiters, fuzzy matching)
- `test/TEST_CSV_README.md` - CSV test documentation and validation procedures

## Grammar and Generated Files

- `SystemRDL.g4` - ANTLR4 grammar file for SystemRDL 2.0 specification
- `SystemRDLLexer.*` - Generated lexer (auto-generated from grammar)
- `SystemRDLParser.*` - Generated parser (auto-generated from grammar)
- `SystemRDLBaseVisitor.*` - Generated base visitor class (auto-generated from grammar)
- `SystemRDLVisitor.*` - Generated visitor interface (auto-generated from grammar)

## Test Resources

- `test/*.rdl` - 16 comprehensive SystemRDL test files covering various language features
  - Basic structures, arrays, parameters, enumerations, memory components
  - Complex expressions, bit ranges, component reuse patterns
  - Register files, field properties, and address mapping scenarios

## Python Validation and Testing Scripts

- `script/rdl_semantic_validator.py` - SystemRDL semantic validation using official compiler
  - Validates SystemRDL files against the official specification
  - Demonstrates elaboration process with detailed node information
  - Shows address maps, property inheritance, and array calculations
  - Supports both single file and batch validation modes

- `script/json_output_validator.py` - JSON output validation and testing framework
  - Validates AST JSON schema and structure compliance
  - Validates elaborated model JSON format and structure
  - Performs end-to-end testing with automatic JSON generation
  - Compares consistency between parser and elaborator outputs
  - Supports individual file validation and batch testing

- `script/csv2rdl_validator.py` - CSV to SystemRDL converter validation suite
  - Three-tier validation: conversion success, syntax validation, content validation
  - Auto-discovers CSV test files using `test_csv_*.csv` naming convention
  - Cross-directory execution with automatic project path detection
  - Comprehensive test coverage: basic, multiline, delimiters, fuzzy matching
  - Professional validation framework with detailed reporting and exit codes

## Development Environment

- `.venv/` - Python virtual environment with required dependencies
  - `systemrdl-compiler` for semantic validation
  - Python 3.13+ environment for running validation scripts
- `.venv/pyvenv.cfg` - Virtual environment configuration
- `requirements.txt` - Python dependencies specification
  - Contains pinned versions of required packages
  - Used for reproducible environment setup
  - Install with: `pip install -r requirements.txt`
