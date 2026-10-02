# Troubleshooting

1. **ANTLR4 runtime library not found**
   - With `USE_SYSTEM_ANTLR4=ON`: install the ANTLR4 C++ runtime library, or configure with
     `cmake .. -DUSE_SYSTEM_ANTLR4=OFF` to download it.
   - With `USE_SYSTEM_ANTLR4=OFF`: the downloaded runtime was not built; see the download entry below.

2. **ANTLR4 version conflicts**
   - Pin the version: `cmake .. -DUSE_SYSTEM_ANTLR4=OFF -DANTLR4_VERSION=4.13.2`.
   - Clear the build directory when switching between system and downloaded ANTLR4: `rm -rf build/*`.

3. **Compilation errors**
   - SystemRDL requires a C++17 or newer compiler.
   - Regenerate the C++ sources: `make generate-antlr4-cpp`.

4. **ANTLR4 download failure**
   - Use a proxy if needed: `export https_proxy=your_proxy`.
   - Fall back to the system runtime: `cmake .. -DUSE_SYSTEM_ANTLR4=ON`.

5. **Automatic reserved field generation**
   - Reserved fields are named `RESERVED_<msb>_<lsb>`, or `RESERVED_<bit>` for a single bit.
   - Check whether a register has gaps using a test file:
     `./build/systemrdl_elaborator test/test_auto_reserved_fields.rdl`
   - `regwidth` must be specified on the register:

     ```systemrdl
     reg example_reg {
         regwidth = 32;
         // ... field definitions
     };
     ```

   - Overlapping fields are reported by the elaborator.
   - Gap detection works with any register width (8, 16, 32, 64, custom).
