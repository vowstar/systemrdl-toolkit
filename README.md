# SystemRDL Toolkit

This is a comprehensive SystemRDL toolkit based on ANTLR4 that provides parsing and elaboration capabilities for
SystemRDL files. The toolkit includes:

- **Parser**: Parses SystemRDL files and generates Abstract Syntax Trees (AST)
- **Elaborator**: Processes and elaborates the parsed SystemRDL descriptions for further analysis and code generation
- **Library**: Use SystemRDL functionality as a library in your C++ projects

## Dependencies

Before building, please ensure the following dependencies are installed:

### System Dependencies

#### Ubuntu/Debian

```bash
sudo apt-get install cmake build-essential pkg-config libantlr4-runtime-dev python3 python3-venv python3-pip
```

#### Gentoo

```bash
sudo emerge cmake dev-util/cmake dev-libs/antlr-cpp python:3.13
```

### Python Dependencies

The project includes Python scripts for validation and testing that require additional dependencies:

#### Set up Python Virtual Environment

The project includes a `requirements.txt` file with all necessary Python dependencies. Follow these steps to set up the
Python environment:

```bash
# 1. Create virtual environment
python3 -m venv .venv

# 2. Activate virtual environment
source .venv/bin/activate

# 3. Upgrade pip to latest version (recommended)
pip install --upgrade pip

# 4. Install all required dependencies from requirements.txt
pip install -r requirements.txt

# 5. Verify installation
python3 -c "import systemrdl; print('SystemRDL compiler version:', systemrdl.__version__)"
```

**For Windows users:**

```cmd
# 2. Activate virtual environment (Windows)
.venv\Scripts\activate

# Other steps remain the same
```

#### Manual Installation (Alternative)

If you prefer to install dependencies manually:

```bash
# Create and activate virtual environment
python3 -m venv .venv
source .venv/bin/activate

# Install specific version (as specified in requirements.txt)
pip install systemrdl-compiler==1.29.3
```

#### Deactivating Virtual Environment

When you're done working with the project:

```bash
# Deactivate virtual environment
deactivate
```

#### Required Python Packages

The `requirements.txt` file contains:

- **systemrdl-compiler==1.29.3**: Official SystemRDL 2.0 compiler for semantic validation
  - Used by `rdl_semantic_validator.py` for validating SystemRDL files
  - Used by the test framework for semantic validation tests
  - Provides the Python API for SystemRDL elaboration and compilation

## Building

The project now features flexible ANTLR4 version management with automatic downloading and building capabilities.

### ANTLR4 Version Control

You can control which ANTLR4 version to use in several ways:

#### 1. Default Version (4.13.2) - Automatic Download

```bash
mkdir build && cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF
make
```

#### 2. Environment Variable

```bash
export ANTLR4_VERSION=4.11.1
mkdir build && cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF
make
```

#### 3. Command Line Parameter

```bash
mkdir build && cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF -DANTLR4_VERSION=4.12.0
make
```

#### 4. Use System-Installed ANTLR4

```bash
# Install system ANTLR4 first (e.g., sudo apt install antlr4-cpp-runtime-dev)
mkdir build && cd build
cmake .. -DUSE_SYSTEM_ANTLR4=ON
make
```

### Version Selection Priority

Version selection follows this priority order:

1. Command line parameter (`-DANTLR4_VERSION=x.y.z`)
2. Environment variable (`ANTLR4_VERSION=x.y.z`)
3. Default version (4.13.2)

### Code Generation

The build system now includes integrated targets for ANTLR4 JAR download and C++ code generation:

```bash
# Download ANTLR4 JAR (uses configured version)
make download-antlr4-jar

# Generate C++ files from SystemRDL.g4
make generate-antlr4-cpp
```

The generated files will include relative paths in comments:

```cpp
// Generated from SystemRDL.g4 by ANTLR 4.x.y
```

### Manual Installation (Legacy)

For manual ANTLR4 setup, you can still follow the traditional approach:

#### Download ANTLR4 JAR

```bash
# Download ANTLR4 JAR file
wget https://www.antlr.org/download/antlr-4.13.2-complete.jar

# Set up ANTLR4 environment (optional, for convenience)
export CLASSPATH=".:antlr-4.13.2-complete.jar:$CLASSPATH"
alias antlr4='java -jar antlr-4.13.2-complete.jar'
alias grun='java org.antlr.v4.gui.TestRig'
```

#### Install ANTLR4 C++ Runtime from Source

```bash
# Download ANTLR4 source code
wget https://www.antlr.org/download/antlr4-cpp-runtime-4.13.2-source.zip
unzip antlr4-cpp-runtime-4.13.2-source.zip
cd antlr4-cpp-runtime-4.13.2-source

# Build and install
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

#### Generate C++ Files Manually

```bash
# Generate C++ lexer, parser, and visitor files from the grammar
java -jar antlr-4.13.2-complete.jar -Dlanguage=Cpp -no-listener -visitor SystemRDL.g4

# This will generate the following files:
# - SystemRDLLexer.h/cpp
# - SystemRDLParser.h/cpp
# - SystemRDLBaseVisitor.h/cpp
# - SystemRDLVisitor.h
```

**Note:** The generated C++ files are required for compilation. With the new build system, these are automatically
managed, but if you modify the `SystemRDL.g4` grammar file, you can regenerate them using `make generate-antlr4-cpp`.

### Standard Build

```bash
mkdir build
cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF  # Use automatic ANTLR4 download
make -j$(nproc)
```

### Build with Tests

The project includes comprehensive testing capabilities through CMake's CTest framework:

```bash
mkdir build
cd build
cmake .. -DUSE_SYSTEM_ANTLR4=OFF
make -j$(nproc)

# Run fast tests (JSON + semantic validation)
make test-fast

# Run all tests
make test-all

# Or using CTest directly
ctest --output-on-failure --verbose
```

### Generated Files

The following files are generated from `SystemRDL.g4`:

- `SystemRDLLexer.cpp/h`
- `SystemRDLParser.cpp/h`
- `SystemRDLBaseVisitor.cpp/h`
- `SystemRDLVisitor.cpp/h`

## CSV to SystemRDL Converter Usage

The toolkit includes a professional CSV to SystemRDL converter (`systemrdl_csv2rdl`) with advanced parsing capabilities
and comprehensive validation.

### Basic Usage

```bash
# Convert CSV file to SystemRDL (auto-generate output filename)
./build/systemrdl_csv2rdl input.csv

# Specify custom output filename
./build/systemrdl_csv2rdl input.csv -o output.rdl

# Display help information
./build/systemrdl_csv2rdl --help
```

### CSV Format Requirements

The converter supports a three-layer structure: **addrmap → reg → field**. CSV files should contain the following
columns (header names are case-insensitive with fuzzy matching support):

| Column | Required | Description | Example |
|--------|----------|-------------|---------|
| `addrmap_offset` | Yes | Address map base offset | `0x0000` |
| `addrmap_name` | Yes | Address map name | `DEMO` |
| `reg_offset` | Yes | Register offset within address map | `0x0000` |
| `reg_name` | Yes | Register name | `CTRL` |
| `reg_width` | Yes | Register width in bits | `32` |
| `field_name` | Yes | Field name | `ENABLE` |
| `field_lsb` | Yes | Field least significant bit | `0` |
| `field_msb` | Yes | Field most significant bit | `0` |
| `reset_value` | Yes | Field reset value | `0` |
| `sw_access` | Yes | Software access type | `RW`/`RO`/`WO` |
| `hw_access` | Yes | Hardware access type | `RW`/`RO`/`WO` |
| `description` | Optional | Field/register description | `Enable control bit` |

### CSV Format Example

See the complete example: [`test/test_csv_basic_example.csv`](test/test_csv_basic_example.csv)

This example demonstrates:

- Three-layer structure (addrmap → reg → field)
- Multi-line descriptions with proper quoting
- Empty fields to indicate hierarchy relationships
- Various access types (RW/RO) for software and hardware

### Advanced Features

#### Intelligent Header Matching

- **Case-insensitive**: `AddrmapOffset` → `addrmap_offset`
- **Fuzzy matching**: Handles typos with Levenshtein distance ≤3
- **Abbreviation support**: `sw_acc` → `sw_access`, `hw_acc` → `hw_access`

#### Multi-line Field Support

The converter handles multi-line descriptions properly:

```csv
field_name,description
MODE,"Operation mode selection
- 0x0: Mode0: Foo bar
- 0x1: Mode1: Foz baz
- 0x2: Mode2: Fooo baar
- 0x3: Reserved"
```

#### Flexible Delimiters

Automatically detects and supports:

- Comma-separated values (`,`)
- Semicolon-separated values (`;`)

#### String Processing

- **Name fields** (addrmap_name, reg_name, field_name): Remove all newlines and trim whitespace
- **Description fields**: Preserve internal newlines, collapse multiple consecutive newlines
- **Regular fields**: Basic trim operations

### Validation and Testing

#### Automated Validation Suite

Run comprehensive validation from any directory:

```bash
# Run complete validation suite
python3 script/csv2rdl_validator.py

# The validator performs three levels of testing:
# 1. CSV2RDL conversion success
# 2. SystemRDL syntax validation (using parser)
# 3. Content pattern validation
```

#### Test Coverage

The validation suite includes 8 comprehensive test scenarios:

- **Basic Example**: Standard CSV conversion
- **Multiline Processing**: Various multi-line field scenarios
- **Delimiter Detection**: Semicolon vs comma delimiter handling
- **Fuzzy Header Matching**: Header name variations and typos
- **Extreme Cases**: Edge cases with special characters

#### Manual Testing

```bash
# Convert and validate manually
./build/systemrdl_csv2rdl test/test_csv_basic_example.csv
./build/systemrdl_parser test/test_csv_basic_example.rdl
```

### Generated SystemRDL Output

```systemrdl
addrmap DEMO {

    reg {
        name = "Control Register";
        desc = "Control Register";
        regwidth = 32;

        field {
            name = "ENABLE";
            desc = "Enable control bit";
            sw = rw;
            hw = rw;
        } ENABLE[0:0] = 0;

        field {
            name = "MODE";
            desc = "Operation mode selection
- 0x0: Mode0: Normal operation
- 0x1: Mode1: Test mode
- 0x2: Mode2: Debug mode
- 0x3: Reserved";
            sw = rw;
            hw = rw;
        } MODE[2:1] = 0;

    } CTRL @ 0x0000;

    reg {
        name = "Status Register";
        desc = "Status Register";
        regwidth = 32;

        field {
            name = "READY";
            desc = "Ready status";
            sw = r;
            hw = r;
        } READY[0:0] = 0;

        field {
            name = "ERROR";
            desc = "Error flag";
            sw = r;
            hw = r;
        } ERROR[1:1] = 0;

    } STATUS @ 0x0004;

};
```

## Code Quality and Development Tools

The project includes comprehensive code quality checking and automatic formatting tools integrated into the CMake build
system. These tools ensure consistent code style and help maintain high code quality across both C++ and Python
components.

### Prerequisites

To use all code quality features, install the following tools:

```bash
# Ubuntu/Debian
sudo apt-get install clang-format cppcheck

# Gentoo
sudo emerge clang dev-util/cppcheck

# Python tools (installed via requirements.txt)
pip install black isort flake8 pymarkdownlnt
```

### Available Quality Targets

Use `make quality-help` (from the build directory) to see all available targets:

```bash
cd build
make quality-help
```

### 🔍 Code Checking Targets

#### Check All Code Quality

```bash
make quality-check          # Run all quality checks (no fixes)
```

#### C++ Code Quality

```bash
make format-check           # Check C++ code formatting with clang-format
make format-diff            # Show C++ formatting differences
make cppcheck               # Run C++ static analysis
make cppcheck-verbose       # Run C++ static analysis (verbose output)
```

#### Python Code Quality

```bash
make python-quality         # Run complete Python quality checks
make python-format-check    # Check Python code formatting
make python-lint            # Run Python linting with flake8
```

#### Markdown Code Quality

```bash
make markdown-lint          # Lint Markdown files with PyMarkdown
make markdown-fix           # Auto-fix Markdown issues with PyMarkdown
make markdown-rules         # Show available markdown linting rules
```

### 🔧 Code Fixing Targets

#### Auto-format All Code

```bash
make quality-format         # Auto-format all code (C++ and Python)
make fix-all               # Fix all auto-fixable issues
```

#### Auto-format Specific Languages

```bash
make format                 # Auto-format C++ code with clang-format
make python-format          # Auto-format Python code with black and isort
```

#### Auto-format Markdown

```bash
make markdown-fix           # Auto-fix Markdown issues with PyMarkdown
```

### 🚀 Development Workflow Targets

#### Pre-commit Checks

```bash
make pre-commit             # Run all quality checks + fast tests (CI equivalent)
```

This target runs the same checks that CI will perform:

- C++ code formatting verification
- C++ static analysis with cppcheck
- Python code quality checks
- Fast test suite (JSON + semantic validation)

#### Complete Quality Analysis

```bash
make quality-all            # Run comprehensive analysis with verbose output
```

### Code Quality Configuration

#### clang-format Configuration

The project uses a comprehensive `.clang-format` configuration file located in the project root. Key formatting rules include:

- **Column Limit**: 100 characters
- **Indentation**: 4 spaces
- **Brace Style**: Custom (Allman-style for functions/classes, K&R for control statements)
- **Alignment**: Consecutive assignments and declarations
- **Pointer Alignment**: Right-aligned (`int *ptr`)

The CI ensures that all C++ code follows this formatting standard using:

```bash
clang-format --style=file:.clang-format --dry-run -Werror
```

#### cppcheck Configuration

Static analysis is configured with:

- **Enabled Checks**: warning, style, performance
- **Suppressions**: Configured in `.cppcheck-suppressions` file
- **Target Files**: `elaborator.cpp`, `elaborator_main.cpp`, `parser_main.cpp`

#### Python Code Quality Standards

- **black**: Code formatting with default settings
- **isort**: Import sorting with black-compatible settings
- **flake8**: Linting for critical errors and style issues
  - Error codes: E9, F63, F7, F82 (critical errors only)
  - Max complexity: 10
  - Max line length: 127 characters

#### Markdown Linting Configuration

The project uses **PyMarkdown** (pymarkdownlnt) for comprehensive Markdown linting. Configuration is stored in `.pymarkdown.json`:

- **Line Length**: 120 characters (MD013)
- **Heading Style**: ATX headings (`#`) preferred (MD003)
- **List Style**: Dash (`-`) for unordered lists (MD004)
- **Code Blocks**: Fenced blocks with backticks preferred (MD046, MD048)
- **Disabled Rules**:
  - MD012: Multiple consecutive blank lines (allows flexibility)
  - MD024: Multiple headings with same content (common in documentation)
  - MD033: HTML in Markdown (needed for complex formatting)
  - MD036: Emphasis possibly used instead of a heading element (disabled as too strict)
  - MD043: Required heading structure (too restrictive for diverse docs)

**Files Excluded from Linting**:

- `.github/` directory (templates and workflows)
- `test/` directory (test data and examples)
- `build/`, `.git/`, `.venv/`, `__pycache__/` (build artifacts and system files)
- `CHANGELOG.md` and `UPDATED_*.md` (special formatting requirements)

The linter supports 46+ rules covering:

- Heading structure and spacing
- List formatting and consistency
- Code block style and syntax
- Link and image validation
- Line length and trailing whitespace
- Emphasis and strong text usage

### Integration with CI

The code quality checks are integrated into the GitHub Actions CI pipeline. The CI runs:

1. **C++ Code Formatting Check**: Verifies all C++ files conform to `.clang-format` style
2. **C++ Static Analysis**: Runs cppcheck with project-specific suppressions
3. **Python Code Quality**: Checks Python formatting and linting standards
4. **Markdown Linting**: Validates Markdown files with PyMarkdown rules

To ensure your changes pass CI, run before committing:

```bash
cd build
make pre-commit
```

### Development Best Practices

#### Before Committing Code

```bash
# 1. Auto-fix formatting issues
make fix-all

# 2. Run pre-commit checks
make pre-commit

# 3. If any issues remain, check individual components
make format-diff           # See C++ formatting differences
make python-quality        # Check Python code quality
make cppcheck-verbose      # See detailed static analysis
```

#### During Development

```bash
# Quick formatting check
make format-check

# Format code automatically
make format

# Run fast tests during development
make test-fast
```

#### Advanced Analysis

```bash
# Comprehensive quality analysis
make quality-all

# Detailed static analysis
make cppcheck-verbose

# Show all formatting differences
make format-diff
```

### Tool Requirements and Fallbacks

The build system gracefully handles missing tools:

- **clang-format not found**: Formatting targets show helpful error messages
- **cppcheck not found**: Static analysis targets show installation instructions
- **Python tools missing**: Install via `pip install black isort flake8`

All tools are automatically detected during CMake configuration and appropriate targets are created.

## Usage

After successful build, the executables are located in the `build/` directory.

### Parser Usage

The parser can display the Abstract Syntax Tree (AST) and optionally export it to JSON format:

```bash
# Parse and print AST to console
./build/systemrdl_parser input.rdl

# Parse and generate JSON output with default filename (input_ast.json)
./build/systemrdl_parser input.rdl --json

# Parse and generate JSON output with custom filename
./build/systemrdl_parser input.rdl --json=my_ast.json

# Short option variant
./build/systemrdl_parser input.rdl -j=output.json
```

### Elaborator Usage

The elaborator processes SystemRDL files through semantic analysis and can export the elaborated model to JSON:

```bash
# Elaborate SystemRDL file and display to console
./build/systemrdl_elaborator input.rdl

# Elaborate and generate JSON output with default filename (input_elaborated.json)
./build/systemrdl_elaborator input.rdl --json

# Elaborate and generate JSON output with custom filename
./build/systemrdl_elaborator input.rdl --json=my_model.json

# Short option variant
./build/systemrdl_elaborator input.rdl -j=output.json
```

### Automatic Gap Detection and Reserved Field Generation

The elaborator automatically detects and fills gaps in register field definitions with reserved fields:

```bash
# Test automatic gap detection with a register containing field gaps
./build/systemrdl_elaborator test/test_auto_reserved_fields.rdl
```

**Example SystemRDL input with gaps**:

```systemrdl
addrmap test_auto_reserved_fields {
    reg gap_reg {
        regwidth = 32;

        field {
            sw = rw;
            hw = r;
            desc = "Control bit";
        } ctrl[0:0];        // bit 0

        // Gap: bits 1-3 are unspecified

        field {
            sw = rw;
            hw = r;
            desc = "Status bits";
        } status[7:4];      // bits 4-7

        // Gap: bits 8-15 are unspecified

        field {
            sw = rw;
            hw = r;
            desc = "Data field";
        } data[23:16];      // bits 16-23

        // Gap: bits 24-30 are unspecified

        field {
            sw = rw;
            hw = r;
            desc = "Enable bit";
        } enable[31:31];    // bit 31
    };

    gap_reg test_reg @ 0x0000;
};
```

**Elaborator output with automatic reserved fields**:

```bash
🔧 reg: test_reg (size: 4 bytes)
  🔧 field: ctrl [0:0]
    📝 width: 1, lsb: 0, msb: 0, sw: "rw"
  🔧 field: RESERVED_3_1 [3:1]    # Automatically generated
    📝 width: 3, lsb: 1, msb: 3, sw: "r"
  🔧 field: status [7:4]
    📝 width: 4, lsb: 4, msb: 7, sw: "rw"
  🔧 field: RESERVED_15_8 [15:8]  # Automatically generated
    📝 width: 8, lsb: 8, msb: 15, sw: "r"
  🔧 field: data [23:16]
    📝 width: 8, lsb: 16, msb: 23, sw: "rw"
  🔧 field: RESERVED_30_24 [30:24] # Automatically generated
    📝 width: 7, lsb: 24, msb: 30, sw: "r"
  🔧 field: enable [31:31]
    📝 width: 1, lsb: 31, msb: 31, sw: "rw"
```

**Features of automatic gap detection**:

- **Intelligent Gap Detection**: Analyzes all field bit ranges to identify unspecified bits
- **Scientific Naming**: Reserved fields use `RESERVED_<msb>_<lsb>` naming convention
- **Proper Properties**: Reserved fields are automatically set to `sw=r, hw=na` (read-only)
- **Complete Coverage**: Ensures all register bits from 0 to (regwidth-1) are covered
- **Any Register Width**: Works with 8-bit, 16-bit, 32-bit, 64-bit, and custom register widths
- **Performance Optimized**: Only processes registers with detected gaps

### Command Line Options

Both tools support the following options:

- `-j, --json[=<filename>]` - Enable JSON output, optionally specify custom filename
- `-h, --help` - Show help message

If no filename is specified with `--json`, the tools automatically generate a filename based on the input file:

- Parser: `<input_basename>_ast.json`
- Elaborator: `<input_basename>_elaborated.json`

### Input/Output Examples

**Input file** (`example.rdl`):

```systemrdl
addrmap simple_chip {
    reg {
        field {
            sw = rw;
        } data[31:0];
    } reg1 @ 0x0;

    reg {
        field {
            sw = rw;
        } status[7:0];
    } reg2 @ 0x4;
};
```

**Parser console output**:

```bash
✅ Parsing successful!

=== Abstract Syntax Tree ===
📦 Component Definition
    🔧 Type: addrmap
        🔧 Type: reg
            🔧 Type: field
              ⚙️ Property: sw=rw
          📋 Instance: data[31:0]
            📏 Range: [31:0]
      📋 Instance: reg1@0x0
        📍 Address: @0x0
        ...
```

**Elaborator console output**:

```bash
🔧 Parsing SystemRDL file: example.rdl
✅ Parsing successful!

🚀 Starting elaboration...
✅ Elaboration successful!

=== Elaborated SystemRDL Model ===
📦 addrmap: simple_chip @ 0x0
  🔧 reg: reg1 (size: 4 bytes)
    🔧 field: data [31:0]
      📝 width: 32
      📝 lsb: 0
      📝 sw: "rw"
      📝 msb: 31
  🔧 reg: reg2 @ 0x4 (size: 4 bytes)
    🔧 field: status @ 0x4 [7:0]
      📝 width: 8
      📝 lsb: 0
      📝 sw: "rw"
      📝 msb: 7

📊 Address Map:
Address     Size    Name      Path
------------------------------------
0x00000000  4       reg1      simple_chip.reg1
0x00000004  4       reg2      simple_chip.reg2
```

**JSON output** (example of elaborated model):

```json
{
  "format": "SystemRDL_ElaboratedModel",
  "version": "1.0",
  "model": [
    {
      "node_type": "addrmap",
      "inst_name": "simple_chip",
      "absolute_address": "0x0",
      "size": 0,
      "children": [
        {
          "node_type": "reg",
          "inst_name": "reg1",
          "absolute_address": "0x0",
          "size": 4,
          "children": [
            {
              "node_type": "field",
              "inst_name": "data",
              "absolute_address": "0x0",
              "size": 0,
              {
                "_section": "properties",
                "width": 32,
                "lsb": 0,
                "sw": "rw",
                "msb": 31
              }
            }
          ]
        }
      ]
    }
  ]
}
```

## Testing

The project includes comprehensive testing capabilities with both C++ tools and Python validation scripts:

### Setup Requirements

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

### Quick Testing

```bash
# Run fast tests (JSON + semantic validation)
cd build
make test-fast

# Run JSON output tests only
make test-json

# Run semantic validation tests only
make test-semantic
```

### Python Validation Scripts

The project includes two main Python scripts for validation:

#### 1. RDL Semantic Validator (`script/rdl_semantic_validator.py`)

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

#### 2. JSON Output Validator (`script/json_output_validator.py`)

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

### Comprehensive Testing

```bash
# Run all tests (parser + elaborator + JSON + semantic)
make test-all

# Standard CTest execution
make test

# Verbose output with details
ctest --output-on-failure --verbose

# Custom target for comprehensive testing
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

The project includes 16 comprehensive test files covering various SystemRDL features:

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

## File Description

### Core C++ Components

- `parser_main.cpp` - Main program for the SystemRDL parser with JSON export capability
- `elaborator_main.cpp` - Main program for the SystemRDL elaborator with JSON export capability
- `elaborator.cpp/.h` - Elaboration engine implementation for semantic analysis
- `cmdline_parser.h` - Command line argument parsing utilities
- `json_output.h` - JSON output formatting and export functions
- `CMakeLists.txt` - CMake build configuration with integrated testing and ANTLR4 management

### CSV to SystemRDL Converter

- `csv2rdl_main.cpp` - CSV to SystemRDL converter with intelligent header matching and multi-line support
- `script/csv2rdl_validator.py` - Comprehensive validation suite for CSV converter testing
- `test/test_csv_*.csv` - CSV test files covering various scenarios (basic, multiline, delimiters, fuzzy matching)
- `test/TEST_CSV_README.md` - CSV test documentation and validation procedures

### Grammar and Generated Files

- `SystemRDL.g4` - ANTLR4 grammar file for SystemRDL 2.0 specification
- `SystemRDLLexer.*` - Generated lexer (auto-generated from grammar)
- `SystemRDLParser.*` - Generated parser (auto-generated from grammar)
- `SystemRDLBaseVisitor.*` - Generated base visitor class (auto-generated from grammar)
- `SystemRDLVisitor.*` - Generated visitor interface (auto-generated from grammar)

### Test Resources

- `test/*.rdl` - 16 comprehensive SystemRDL test files covering various language features
  - Basic structures, arrays, parameters, enumerations, memory components
  - Complex expressions, bit ranges, component reuse patterns
  - Register files, field properties, and address mapping scenarios

### Python Validation and Testing Scripts

- `script/rdl_semantic_validator.py` - SystemRDL semantic validation using official compiler
  - Validates SystemRDL files against the official specification
  - Demonstrates elaboration process with detailed node information
  - Shows address maps, property inheritance, and array calculations
  - Supports both single file and batch validation modes

- `script/json_output_validator.py` - JSON output validation and testing framework
  - Validates AST JSON schema and structure compliance
  - Validates elaborated model JSON format and content
  - Performs end-to-end testing with automatic JSON generation
  - Compares consistency between parser and elaborator outputs
  - Supports individual file validation and batch testing

- `script/csv2rdl_validator.py` - CSV to SystemRDL converter validation suite
  - Three-tier validation: conversion success, syntax validation, content validation
  - Auto-discovers CSV test files using `test_csv_*.csv` naming convention
  - Cross-directory execution with automatic project path detection
  - Comprehensive test coverage: basic, multiline, delimiters, fuzzy matching
  - Professional validation framework with detailed reporting and exit codes

### Development Environment

- `.venv/` - Python virtual environment with required dependencies
  - `systemrdl-compiler` for semantic validation
  - Python 3.13+ environment for running validation scripts
- `.venv/pyvenv.cfg` - Virtual environment configuration
- `requirements.txt` - Python dependencies specification
  - Contains pinned versions of required packages
  - Used for reproducible environment setup
  - Install with: `pip install -r requirements.txt`

## Features

- **Complete SystemRDL 2.0 Support**: Full implementation of the SystemRDL 2.0 specification
- **Flexible ANTLR4 Version Management**: Support for multiple ANTLR4 versions with automatic download
  - Environment variable support (`ANTLR4_VERSION`)
  - Command line parameter support (`-DANTLR4_VERSION=x.y.z`)
  - Automatic JAR download and C++ code generation
  - System ANTLR4 runtime integration
- **AST Generation**: Detailed Abstract Syntax Tree representation of SystemRDL designs
- **Elaboration Engine**: Semantic analysis and elaboration of SystemRDL descriptions
- **JSON Export**: Export parsed AST and elaborated model to structured JSON format
  - Standardized JSON schema for interoperability
  - Support for both AST and elaborated model formats
  - Automatic filename generation or custom file specification
- **Flexible Command Line Interface**: Modern CLI with optional parameters
  - Support for `--json` (default filename) and `--json=custom.json` formats
  - Short options (`-j`) and long options (`--json`) support
  - Future-ready for multiple output formats (e.g., `--yaml`, `--xml`)
- **Comprehensive Error Reporting**: Detailed error detection and reporting with line/column information
- **Address Map Generation**: Automatic generation of memory address maps from elaborated models
- **CSV to SystemRDL Converter**: Professional-grade CSV to SystemRDL conversion tool
  - **Intelligent Header Matching**: Case-insensitive, fuzzy matching with Levenshtein distance (≤3)
  - **Multi-line CSV Support**: Handles quoted multi-line cells with proper field processing
  - **Flexible Delimiters**: Automatic detection of comma and semicolon separators
  - **Three-layer Structure**: Complete addrmap → reg → field hierarchy support
  - **Access Control Mapping**: Separate software/hardware access permissions (RW/RO/WO)
  - **Smart String Processing**: Field-specific newline and whitespace handling
  - **Validation Framework**: Three-tier validation with comprehensive test coverage
  - **Cross-directory Execution**: Path-agnostic script execution from any directory
- **Python Validation Framework**: Comprehensive validation using official SystemRDL tools
  - **Semantic Validation**: Uses official `systemrdl-compiler` for specification compliance
  - **JSON Schema Validation**: Validates output format and structure
  - **End-to-End Testing**: Automated testing pipeline for complete workflow validation
  - **Batch Processing**: Support for validating multiple files simultaneously
- **Extensible Architecture**: Modular design for easy extension and customization
- **Integrated Testing**: CMake-based testing framework with comprehensive test coverage
  - JSON output validation tests
  - Semantic validation using Python SystemRDL compiler
  - Fast test targets for rapid development
  - 16 comprehensive test files covering all major SystemRDL features
- **Cross-platform Support**: Compatible with Linux, macOS, and Windows
- **Unicode Emoji Interface**: User-friendly console output with emoji indicators
- **Property Analysis**: Complete property inheritance and evaluation system
- **Automatic Gap Detection and Reserved Field Generation**: Intelligent field gap analysis
  - **Gap Detection**: Automatically detects unspecified bit ranges in register definitions
  - **Reserved Field Generation**: Creates appropriately named reserved fields for detected gaps
  - **Scientific Naming**: Uses systematic naming convention `RESERVED_<msb>_<lsb>` for clarity
  - **Register Width Support**: Works with registers of any width (8-bit, 16-bit, 32-bit, 64-bit, custom widths)
  - **Smart Coverage Validation**: Validates field coverage and fills missing bit ranges
  - **Zero-overhead**: Only activates when gaps are detected, preserving performance
- **Comprehensive Field Validation**: Advanced field integrity checking
  - **Field Overlap Detection**: Identifies and reports overlapping bit ranges between fields in the same register
  - **Field Boundary Validation**: Ensures fields do not exceed register width boundaries
  - **Intelligent Error Reporting**: Provides precise error messages with specific bit ranges and field names
  - **Multi-level Validation**: Validates fields before gap detection to ensure data integrity
- **Virtual Environment Integration**: Isolated Python dependencies for reliable testing

## Troubleshooting

1. **ANTLR4 runtime library not found**
   - If using system ANTLR4 (`USE_SYSTEM_ANTLR4=ON`): Ensure ANTLR4 C++ runtime library is installed
   - If using downloaded ANTLR4 (`USE_SYSTEM_ANTLR4=OFF`): The system will automatically download and build it
   - Check if the library is in standard paths, or use `cmake .. -DUSE_SYSTEM_ANTLR4=OFF` for automatic management

2. **Version conflicts**
   - Use `cmake .. -DUSE_SYSTEM_ANTLR4=OFF -DANTLR4_VERSION=4.13.2` to specify exact version
   - Clear build directory if switching between system and downloaded ANTLR4: `rm -rf build/*`

3. **Compilation errors**
   - Ensure using C++17 or higher version compiler
   - If using custom ANTLR4 version, ensure it's compatible (4.9.0+)
   - Regenerate C++ files if needed: `make generate-antlr4-cpp`

4. **Network issues during download**
   - Check internet connection for ANTLR4 JAR and runtime download
   - Use proxy if necessary: `export https_proxy=your_proxy`
   - Fallback to system ANTLR4: `cmake .. -DUSE_SYSTEM_ANTLR4=ON`

5. **Python environment issues**
   - Ensure Python 3.13+ is installed: `python3 --version`
   - Set up virtual environment if missing:

     ```bash
     python3 -m venv .venv
     source .venv/bin/activate
     pip install --upgrade pip
     pip install -r requirements.txt
     ```

   - If requirements.txt installation fails, ensure pip is updated: `pip install --upgrade pip`
   - Verify installation: `python3 -c "import systemrdl; print('SystemRDL available')"`

6. **Semantic validation failures**
   - Check if SystemRDL file syntax is correct using the validator:

     ```bash
     python3 script/rdl_semantic_validator.py test/test_minimal.rdl
     ```

   - Ensure virtual environment is activated before running tests
   - If `ModuleNotFoundError: No module named 'systemrdl'`, install dependencies:

     ```bash
     source .venv/bin/activate
     pip install -r requirements.txt
     ```

7. **JSON validation failures**
   - Run JSON validator directly to see detailed error messages:

     ```bash
     python3 script/json_output_validator.py --ast output_ast.json
     ```

   - Check JSON file permissions and format
   - Ensure C++ tools generated valid JSON output

8. **Runtime errors**
   - Ensure input SystemRDL file has correct syntax
   - Check if file path is correct
   - Use verbose mode for debugging: `./systemrdl_parser file.rdl --verbose`

9. **Test failures**
   - Run individual tests to identify specific issues: `ctest -R "test_name" --output-on-failure`
   - Check test file syntax and ensure it complies with SystemRDL 2.0 specification
   - Use `make test-fast` for quick validation during development
   - For Python-related test failures, check virtual environment status:

     ```bash
     source .venv/bin/activate
     python3 -c "import systemrdl; print('SystemRDL available')"
     ```

10. **Permission issues**
    - Ensure virtual environment directory has proper permissions
    - On some systems, may need to recreate virtual environment:

      ```bash
      rm -rf .venv
      python3 -m venv .venv
      source .venv/bin/activate
      pip install --upgrade pip
      pip install -r requirements.txt
      ```

11. **Automatic reserved field generation issues**
    - **Reserved fields not appearing**: Check if register has actual gaps using test files:

      ```bash
      ./build/systemrdl_elaborator test/test_auto_reserved_fields.rdl
      ```

    - **Incorrect field naming**: Reserved fields use `RESERVED_<msb>_<lsb>` format automatically
    - **Register width issues**: Ensure `regwidth` property is correctly specified:

      ```systemrdl
      reg example_reg {
          regwidth = 32;  // Must specify register width
          // ... field definitions
      };
      ```

    - **Overlapping fields**: Check for field bit range conflicts - elaborator will detect overlaps
    - **Custom register widths**: Gap detection works with any register width (8, 16, 32, 64, custom)
    - **Performance concerns**: Gap detection only activates when gaps are found, no overhead for complete registers

## Acknowledgments

The SystemRDL grammar file (`SystemRDL.g4`) used in this project is derived from the
[SystemRDL Compiler](https://github.com/SystemRDL/systemrdl-compiler) project. We would like to express our sincere
gratitude to the SystemRDL organization and all contributors to the SystemRDL Compiler project for providing the
comprehensive SystemRDL 2.0 language specification and grammar implementation.

The SystemRDL Compiler project is an open-source, MIT-licensed compiler front-end for Accellera's SystemRDL 2.0
register description language, which serves as an excellent foundation for SystemRDL language processing and tool
development.

For more information about the SystemRDL Compiler project, please visit:
[https://github.com/SystemRDL/systemrdl-compiler](https://github.com/SystemRDL/systemrdl-compiler)

## Library Usage

This toolkit has been refactored to provide both a standalone library (`libsystemrdl`) and command-line tools.
The library enables easy integration of SystemRDL parsing and elaboration capabilities into other C++ projects.

### Library Features

- **📚 Library Support**: Use SystemRDL functionality as a library in your C++ projects
- **🔧 Command-line Tools**: Traditional command-line tools for parsing and elaboration
- **🏗️ Flexible Build**: Choose between shared/static libraries and optional components
- **📦 Modern CMake**: Full CMake package support with `find_package()` integration
- **🔍 Code Quality**: Comprehensive testing and code quality tools

### Build Options

The project provides several build options to customize what gets built:

| Option | Default | Description |
|--------|---------|-------------|
| `SYSTEMRDL_BUILD_SHARED` | `ON` | Build shared library |
| `SYSTEMRDL_BUILD_STATIC` | `ON` | Build static library |
| `SYSTEMRDL_BUILD_TOOLS` | `ON` | Build command-line tools |
| `SYSTEMRDL_BUILD_TESTS` | `ON` | Build tests |
| `USE_SYSTEM_ANTLR4` | `OFF` | Use system ANTLR4 instead of downloading |

### Building the Library

#### Basic Library Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### Library-Only Build

If you only need the library (no command-line tools):

```bash
cmake .. -DSYSTEMRDL_BUILD_TOOLS=OFF -DSYSTEMRDL_BUILD_TESTS=OFF
make -j$(nproc)
```

#### Installation

```bash
# Install to default location (/usr/local)
sudo make install

# Or install to custom location
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/systemrdl
make install
```

### Using the Library in Your Project

#### Method 1: CMake find_package (Recommended)

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)

# Find SystemRDL library
find_package(SystemRDL REQUIRED)

# Create your application
add_executable(my_app main.cpp)

# Link against SystemRDL
target_link_libraries(my_app PRIVATE SystemRDL::systemrdl)
```

#### Method 2: pkg-config

```bash
# Check if library is found
pkg-config --exists systemrdl && echo "SystemRDL library found"

# Compile with pkg-config
g++ -std=c++17 main.cpp $(pkg-config --cflags --libs systemrdl) -o my_app
```

#### Method 3: Direct linking

```cmake
target_link_libraries(my_app PRIVATE systemrdl)
target_include_directories(my_app PRIVATE /usr/local/include/systemrdl)
```

### Library API Usage

#### Basic Example

```cpp
#include <systemrdl/elaborator.h>
#include <systemrdl/SystemRDLLexer.h>
#include <systemrdl/SystemRDLParser.h>
#include <antlr4-runtime.h>

using namespace antlr4;
using namespace systemrdl;

int main() {
    // Parse SystemRDL file
    std::ifstream stream("design.rdl");
    ANTLRInputStream input(stream);
    SystemRDLLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    SystemRDLParser parser(&tokens);
    auto tree = parser.root();

    // Elaborate the design
    Elaborator elaborator;
    auto root_node = elaborator.elaborate(tree);

    if (elaborator.has_errors()) {
        // Handle errors
        for (const auto& error : elaborator.get_errors()) {
            std::cerr << "Error: " << error.message << std::endl;
        }
        return 1;
    }

    // Use the elaborated design
    std::cout << "Design: " << root_node->inst_name << std::endl;
    std::cout << "Type: " << root_node->get_node_type() << std::endl;
    std::cout << "Size: " << root_node->size << " bytes" << std::endl;

    return 0;
}
```

#### Working with Address Maps

```cpp
// Create address map visitor
AddressMapVisitor addr_visitor;
root_node->accept_visitor(addr_visitor);

// Get address layout
const auto& address_map = addr_visitor.get_address_map();
for (const auto& entry : address_map) {
    std::cout << "0x" << std::hex << entry.address
              << ": " << entry.name
              << " (" << std::dec << entry.size << " bytes)"
              << std::endl;
}
```

### Available Targets

#### Library Targets

- **`SystemRDL::systemrdl`** - Generic target (shared if available, otherwise static)
- **`SystemRDL::systemrdl_shared`** - Shared library
- **`SystemRDL::systemrdl_static`** - Static library

#### Command-line Tools (if enabled)

- **`systemrdl_parser`** - Parse SystemRDL files and generate AST
- **`systemrdl_elaborator`** - Parse and elaborate SystemRDL designs
- **`systemrdl_csv2rdl`** - Convert CSV files to SystemRDL format

### Library Components

#### Core Classes

| Class | Description |
|-------|-------------|
| `Elaborator` | Main elaboration engine |
| `ElaboratedNode` | Base class for elaborated elements |
| `ElaboratedAddrmap` | Address map component |
| `ElaboratedRegfile` | Register file component |
| `ElaboratedReg` | Register component |
| `ElaboratedField` | Field component |
| `ElaboratedMem` | Memory component |

#### Visitors

| Visitor | Purpose |
|---------|---------|
| `ElaboratedNodeVisitor` | Base visitor interface |
| `AddressMapVisitor` | Generate address map |

#### Utilities

| Component | Description |
|-----------|-------------|
| `PropertyValue` | Property value container |
| `ParameterDefinition` | Parameter definitions |
| `EnumDefinition` | Enumeration definitions |
| `StructDefinition` | Structure definitions |

### Example Project

See the `example/` directory for a complete working example showing library usage.

To build and run the example:

```bash
# First install the library
cmake .. && make && sudo make install

# Then build the example
cd example
mkdir build && cd build
cmake ..
make

# Run the example
./example_app ../test_example.rdl
```

### Library Dependencies

#### Required

- **CMake** 3.16 or later
- **C++17** compatible compiler
- **ANTLR4 C++ runtime** (automatically downloaded if not using system version)

#### Optional

- **Python 3** (for testing and validation)
- **pkg-config** (for pkg-config support)
- **clang-format** (for code formatting)
- **cppcheck** (for static analysis)

### Advanced Library Configuration

#### Custom ANTLR4 Version

```bash
# Use specific ANTLR4 version
cmake .. -DANTLR4_VERSION=4.12.0

# Use system ANTLR4
cmake .. -DUSE_SYSTEM_ANTLR4=ON
```

#### Install Locations

```bash
# Custom install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/systemrdl

# Custom library directory
cmake .. -DCMAKE_INSTALL_LIBDIR=lib64
```

#### Build Types

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Release with debug info
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

### Library Troubleshooting

#### Library Not Found

```bash
# Check installation
find /usr/local -name "*systemrdl*"

# Set custom search path
cmake .. -DCMAKE_PREFIX_PATH=/opt/systemrdl

# Verify with pkg-config
pkg-config --exists systemrdl && echo "Found"
```

#### Compilation Issues

```bash
# Verbose build output
make VERBOSE=1

# Check compiler requirements
g++ --version  # Should support C++17

# Check ANTLR4 installation
find /usr -name "antlr4-runtime*"
```

#### Runtime Issues

```bash
# Check shared library path
ldd your_app

# Set LD_LIBRARY_PATH if needed
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

#### Development

##### Code Quality

```bash
# Check code formatting
make format-check

# Auto-format code
make format

# Run static analysis
make cppcheck

# Run all quality checks
make quality-check-all
```

##### Testing

```bash
# Run all tests
make test

# Run specific test categories
make test-parser
make test-elaborator
make test-json
```

#### Version Information

- **Version**: 1.0.0
- **SystemRDL Standard**: 2.0
- **ANTLR4 Version**: 4.13.2 (default)
- **C++ Standard**: C++17
