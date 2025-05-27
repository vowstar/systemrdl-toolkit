# Contributing to SystemRDL Toolbox

Thank you for your interest in contributing to the SystemRDL Toolbox! This document provides guidelines and information for contributors.

## 🎯 Project Overview

The SystemRDL Toolbox is a C++ implementation of a SystemRDL 2.0 compiler that provides:

- High-performance SystemRDL parsing and elaboration
- 100% compatibility with the official SystemRDL specification
- Advanced validation features including instance address overlap detection
- JSON output for integration with other tools

## 🚀 Getting Started

### Prerequisites

- **C++ Compiler**: GCC 8+ or Clang 10+ with C++17 support
- **CMake**: Version 3.16 or higher
- **Java**: JDK 11+ (required for ANTLR4)
- **Python**: 3.7+ (for testing and comparison scripts)

### Development Setup

1. **Clone the repository**:

   ```bash
   git clone https://github.com/vowstar/systemrdltoolbox.git
   cd systemrdltoolbox
   ```

2. **Install dependencies**:

   ```bash
   # Ubuntu/Debian
   sudo apt-get install openjdk-11-jdk cmake build-essential uuid-dev

   # macOS
   brew install openjdk@11 cmake ossp-uuid

   # Python dependencies
   pip install -r requirements.txt
   ```

3. **Build the project**:

   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --parallel 4
   ```

4. **Run tests**:

   ```bash
   # Basic functionality test
   ./build/systemrdl_elaborator --help

   # Run compatibility tests
   cd script
   python compare_implementations.py
   ```

## 📋 How to Contribute

### Reporting Issues

When reporting issues, please include:

- **SystemRDL file** that reproduces the issue (if applicable)
- **Error messages** or unexpected output
- **Environment details** (OS, compiler version, CMake version)
- **Steps to reproduce** the issue

Use our issue templates for:

- 🐛 Bug Reports
- ✨ Feature Requests
- 📚 Documentation Improvements

### Submitting Pull Requests

1. **Fork** the repository
2. **Create a feature branch**:

   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes** following our coding standards
4. **Add tests** for new functionality
5. **Run the test suite**:

   ```bash
   cd script
   python compare_implementations.py
   ```

6. **Commit your changes** with descriptive messages
7. **Push** to your fork and submit a pull request

### Code Style Guidelines

#### C++ Code Style

- **Standard**: C++17
- **Formatting**: Use `clang-format` with the project's configuration
- **Naming conventions**:
  - Classes: `PascalCase` (e.g., `SystemRDLElaborator`)
  - Functions: `snake_case` (e.g., `process_register`)
  - Variables: `snake_case` (e.g., `register_width`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_FIELD_WIDTH`)

```cpp
// Good
class FieldValidator {
public:
    bool validate_field_boundaries(const FieldNode* field);

private:
    size_t max_field_width_;
    static const size_t DEFAULT_REGISTER_WIDTH = 32;
};

// Bad
class fieldValidator {
public:
    bool ValidateFieldBoundaries(const FieldNode* Field);

private:
    size_t maxFieldWidth;
    static const size_t default_register_width = 32;
};
```

#### Python Code Style

- **Standard**: PEP 8
- **Formatting**: Use `black` and `isort`
- **Type hints**: Use type hints for function signatures
- **Documentation**: Use docstrings for all public functions

```python
# Good
def validate_systemrdl_file(file_path: str) -> bool:
    """Validate a SystemRDL file for syntax and semantic errors.

    Args:
        file_path: Path to the SystemRDL file to validate

    Returns:
        True if the file is valid, False otherwise

    Raises:
        FileNotFoundError: If the file doesn't exist
    """
    pass
```

### Testing Guidelines

#### Adding New Tests

1. **Unit tests**: Add tests for individual functions
2. **Integration tests**: Add SystemRDL test files to `test/` directory
3. **Regression tests**: Ensure new features don't break existing functionality

#### Test File Naming

- SystemRDL test files: `test_<feature>_<description>.rdl`
- Expected behavior markers:
  - `// EXPECT_ELABORATION_FAILURE` - file should fail elaboration
  - `// EXPECT_ELABORATION_SUCCESS` - file should pass elaboration

Example:

```systemrdl
// test_field_overlap_validation.rdl
// EXPECT_ELABORATION_FAILURE

addrmap test {
    reg {
        field {} f1[7:0];
        field {} f2[3:0];  // Overlaps with f1
    } r1;
};
```

#### Running Tests

```bash
# Run all comparison tests
cd script
python compare_implementations.py

# Run JSON validation tests
python json_output_validator.py

# Run specific test file
../build/systemrdl_elaborator test/test_specific_feature.rdl
```

### Documentation

- **README**: Update if adding new features or changing build process
- **Code documentation**: Add meaningful comments for complex logic
- **API documentation**: Document all public interfaces
- **Examples**: Include usage examples for new features

## 🏗️ Project Structure

```bash
systemrdltoolbox/
├── elaborator.h              # Main elaborator header
├── elaborator.cpp            # Main elaborator implementation
├── elaborator_main.cpp       # Elaborator executable
├── parser_main.cpp           # Parser executable
├── CMakeLists.txt            # Build configuration
├── SystemRDL.g4             # ANTLR4 grammar file
├── script/                   # Python testing scripts
│   ├── compare_implementations.py
│   ├── rdl_semantic_validator.py
│   └── json_output_validator.py
├── test/                     # SystemRDL test files
├── .github/                  # GitHub workflows and templates
└── docs/                     # Additional documentation
```

## 🔧 Development Workflow

### Making Changes

1. **Understand the SystemRDL specification**: Familiarize yourself with SystemRDL 2.0
2. **Review existing code**: Understand the current implementation
3. **Plan your changes**: Consider impact on compatibility and performance
4. **Implement incrementally**: Make small, focused changes
5. **Test thoroughly**: Ensure compatibility with existing tests

### Debugging Tips

1. **Enable debug output**:

   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   ```

2. **Use test-driven development**:

   ```bash
   # Create test case first
   echo 'addrmap test { /* your test case */ };' > test/test_new_feature.rdl

   # Run and verify it fails as expected
   ./build/systemrdl_elaborator test/test_new_feature.rdl

   # Implement feature
   # Run test again to verify it passes
   ```

3. **Compare with Python implementation**:

   ```bash
   cd script
   python rdl_semantic_validator.py ../test/test_new_feature.rdl
   ```

### Performance Considerations

- **Memory management**: Use RAII and smart pointers
- **Avoid unnecessary copies**: Use references and move semantics
- **Profile critical paths**: Use profiling tools for optimization
- **Benchmark against Python**: Ensure C++ implementation maintains performance advantage

## 🎯 Contribution Areas

We welcome contributions in these areas:

### High Priority

- 🐛 **Bug fixes**: Address reported issues
- ✅ **Test coverage**: Add more test cases
- 📚 **Documentation**: Improve code and user documentation
- 🔧 **Build system**: Improve CMake configuration

### Medium Priority

- ⚡ **Performance optimizations**: Make the elaborator faster
- 🆕 **New SystemRDL features**: Implement additional language features
- 🔍 **Enhanced validation**: Add more semantic checks
- 🎨 **Error messages**: Improve error reporting

### Nice to Have

- 🌐 **Platform support**: Windows/macOS improvements
- 🔌 **Tool integration**: IDE plugins, language servers
- 📊 **Analytics**: Code metrics and reporting
- 🎯 **Examples**: More comprehensive examples

## 🤝 Community Guidelines

- **Be respectful**: Treat all contributors with respect
- **Be constructive**: Provide helpful feedback and suggestions
- **Be patient**: Review processes take time
- **Ask questions**: Don't hesitate to ask for clarification
- **Share knowledge**: Help others learn and contribute

## 📞 Getting Help

- **GitHub Issues**: For bug reports and feature requests
- **GitHub Discussions**: For questions and general discussion
- **Email**: For security issues or private concerns

## 🏆 Recognition

Contributors are recognized in:

- **CONTRIBUTORS.md**: List of all contributors
- **Release notes**: Major contributions highlighted
- **GitHub contributors graph**: Automatic recognition

Thank you for contributing to the SystemRDL Toolbox! 🎉
