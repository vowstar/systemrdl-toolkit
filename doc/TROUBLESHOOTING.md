
# Troubleshooting

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
