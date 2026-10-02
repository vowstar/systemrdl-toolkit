# Code Quality and Development Tools

Code quality checking and formatting tools are integrated into the CMake build system for the C++ and Python
components.

## Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install clang-format cppcheck

# Gentoo
sudo emerge clang dev-util/cppcheck
```

The Python tools come from `requirements.txt`; see [BUILD.md](BUILD.md).

## Available Quality Targets

Use `make quality-help` from the build directory to list every target:

```bash
cd build
make quality-help
```

## Code Checking Targets

```bash
make quality-check          # Run all quality checks (no fixes)
make format-check           # Check C++ formatting with clang-format
make cppcheck               # Run C++ static analysis
make python-quality         # Check Python formatting and linting
make markdown-lint          # Lint Markdown files with PyMarkdown
```

The remaining per-tool targets (`format-diff`, `cppcheck-verbose`, `python-format-check`, `python-lint`,
`markdown-rules`, `quality-all`) are listed by `make quality-help`.

## Code Fixing Targets

```bash
make fix-all                # Fix all auto-fixable issues (C++, Python, Markdown)
make format                 # Auto-format C++ code with clang-format
make python-format          # Auto-format Python code with black and isort
make markdown-fix           # Auto-fix Markdown issues with PyMarkdown
```

## Pre-commit Checks

```bash
make pre-commit             # Run what CI checks (quality-check + test-fast)
```

If a check fails, `make fix-all` repairs what can be repaired automatically. Use the per-tool targets from
`make quality-help` to see the remaining details.

## Code Quality Configuration

Each tool reads its settings from one file:

| Tool | Configuration file |
| -- | -- |
| clang-format | `.clang-format` |
| cppcheck | `.cppcheck-suppressions` |
| black, isort | `pyproject.toml` |
| flake8 | `.flake8` |
| PyMarkdown | `.pymarkdown.json` |

CI verifies C++ formatting with:

```bash
clang-format --style=file:.clang-format --dry-run -Werror
```

CI installs LLVM 22 (`clang-format-22`), because other major versions format
the same input differently. Match that version locally or the check will
disagree with the formatter.

## Integration with CI

The checks above run in the GitHub Actions pipeline. Run `make pre-commit` before pushing.

## Tool Requirements and Fallbacks

The build system degrades gracefully when a tool is missing:

- **clang-format not found**: the formatting targets print an error and fail.
- **cppcheck not found**: the static-analysis targets print installation instructions and fail.
- **Python or PyMarkdown not found**: the matching targets print an error and fail.

Tools are detected during CMake configuration, which creates the corresponding targets.
