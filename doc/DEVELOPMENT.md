# Code Quality and Development Tools

The project includes code quality checking and automatic formatting tools integrated into the CMake build
system. These tools ensure consistent code style and help maintain high code quality across both C++ and Python
components.

## Prerequisites

To use all code quality features, install the following tools:

```bash
# Ubuntu/Debian
sudo apt-get install clang-format cppcheck

# Gentoo
sudo emerge clang dev-util/cppcheck

# Python tools (installed via requirements.txt)
pip install black isort flake8 pymarkdownlnt
```

## Available Quality Targets

Use `make quality-help` (from the build directory) to see all available targets:

```bash
cd build
make quality-help
```

## Code Checking Targets

### Check All Code Quality

```bash
make quality-check          # Run all quality checks (no fixes)
```

### C++ Code Quality

```bash
make format-check           # Check C++ code formatting with clang-format
make format-diff            # Show C++ formatting differences
make cppcheck               # Run C++ static analysis
make cppcheck-verbose       # Run C++ static analysis (verbose output)
```

### Python Code Quality

```bash
make python-quality         # Run complete Python quality checks
make python-format-check    # Check Python code formatting
make python-lint            # Run Python linting with flake8
```

### Markdown Code Quality

```bash
make markdown-lint          # Lint Markdown files with PyMarkdown
make markdown-fix           # Auto-fix Markdown issues with PyMarkdown
make markdown-rules         # Show available markdown linting rules
```

## Code Fixing Targets

### Auto-format All Code

```bash
make quality-format         # Auto-format all code (C++ and Python)
make fix-all               # Fix all auto-fixable issues
```

### Auto-format Specific Languages

```bash
make format                 # Auto-format C++ code with clang-format
make python-format          # Auto-format Python code with black and isort
```

### Auto-format Markdown

```bash
make markdown-fix           # Auto-fix Markdown issues with PyMarkdown
```

## Development Workflow Targets

### Pre-commit Checks

```bash
make pre-commit             # Run all quality checks + fast tests (CI equivalent)
```

This target runs the same checks that CI will perform:

- C++ code formatting verification
- C++ static analysis with cppcheck
- Python code quality checks
- Fast test suite (JSON + semantic validation)

### Complete Quality Analysis

```bash
make quality-all            # Run analysis with verbose output
```

## Code Quality Configuration

### clang-format Configuration

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

### cppcheck Configuration

Static analysis is configured with:

- **Enabled Checks**: warning, style, performance
- **Suppressions**: Configured in `.cppcheck-suppressions` file
- **Target Files**: `elaborator.cpp`, `elaborator_main.cpp`, `parser_main.cpp`

### Python Code Quality Standards

- **black**: Code formatting with default settings
- **isort**: Import sorting with black-compatible settings
- **flake8**: Linting for critical errors and style issues
  - Error codes: E9, F63, F7, F82 (critical errors only)
  - Max complexity: 10
  - Max line length: 127 characters

### Markdown Linting Configuration

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

## Integration with CI

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

## Development Best Practices

### Before Committing Code

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

### During Development

```bash
# Quick formatting check
make format-check

# Format code automatically
make format

# Run fast tests during development
make test-fast
```

### Analysis

```bash
# Comprehensive quality analysis
make quality-all

# Detailed static analysis
make cppcheck-verbose

# Show all formatting differences
make format-diff
```

## Tool Requirements and Fallbacks

The build system gracefully handles missing tools:

- **clang-format not found**: Formatting targets show helpful error messages
- **cppcheck not found**: Static analysis targets show installation instructions
- **Python tools missing**: Install via `pip install black isort flake8`

All tools are automatically detected during CMake configuration and appropriate targets are created.
