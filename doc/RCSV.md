# RCSV (Register-CSV) Specification v0.4

RCSV is a practical, field-oriented CSV format for SystemRDL Toolkit. It provides a standardized way to represent register maps that can be converted to SystemRDL without semantic loss.

**Design Principles:**

- **Simplicity**: Easy to understand and create manually or programmatically
- **Direct Mapping**: CSV structure maps directly to SystemRDL syntax
- **Completeness**: Preserves all essential register/field information
- **Validation**: Clear error messages for malformed input

---

## 1. Purpose and Scope

RCSV addresses the need for a standardized CSV format for register map interchange:

- **Primary Use**: Converting CSV register specifications to SystemRDL
- **Target Audience**: Hardware engineers, verification engineers, documentation teams
- **Scope**: Elaborated register maps with resolved addresses, widths, and properties
- **Single Address Map**: One RCSV file describes exactly one address map (for multiple address maps, use separate files)
- **Compatibility**: Designed to work with SystemRDL Toolkit's existing CSV2RDL converter

---

## 2. File Format and Encoding

RCSV follows standard CSV conventions with specific requirements:

- **Encoding**: UTF-8 with Unix line endings (`\n`)
- **Structure**: Comma-separated values with **mandatory header row**
- **Delimiter**: Standard comma (`,`) - semicolon (`;`) support is optional for compatibility
- **Quoting**: Multi-line cells supported with double quotes (`"`)
- **Escaping**: Double quotes in cells escaped as `""` (RFC 4180 compliant)

---

## 3. Structure Overview

RCSV uses a **row-based hierarchical structure** compatible with existing CSV tools:

- **Header Row**: Defines column names. Column names MUST match exactly and are case-sensitive.
- **Address Map Row**: Defines the top-level address map container
- **Register Rows**: Define individual registers within the address map
- **Field Rows**: Define fields within each register (one row per field)

This structure maintains compatibility with the SystemRDL Toolkit's current CSV2RDL implementation while providing standardization.

---

## 4. Required Columns

RCSV defines a standard set of columns compatible with existing CSV2RDL tools:

### 4.1 Core Identification Columns

|      Column      | Required |                   Description                    |   Example   |
| ---------------- | -------- | ------------------------------------------------ | ----------- |
| `addrmap_offset` | Yes      | Address map base offset (hex/decimal)            | `0x0000`    |
| `addrmap_name`   | Yes      | Address map instance name                        | `DEMO_CHIP` |
| `reg_offset`     | Yes      | Register offset within address map (hex/decimal) | `0x1000`    |
| `reg_name`       | Yes      | Register instance name                           | `CTRL_REG`  |
| `reg_width`      | Yes      | Register width in bits                           | `32`        |

### 4.2 Field Definition Columns

|    Column     | Required |             Description              |   Example    |
| ------------- | -------- | ------------------------------------ | ------------ |
| `field_name`  | Yes      | Field instance name                  | `ENABLE`     |
| `field_lsb`   | Yes      | Field least significant bit position | `0`          |
| `field_msb`   | Yes      | Field most significant bit position  | `3`          |
| `reset_value` | Yes      | Field reset value (decimal/hex)      | `5` or `0x5` |

### 4.3 Access Control Columns

|   Column    | Required |         Description         |      Valid Values      |
| ----------- | -------- | --------------------------- | ---------------------- |
| `sw_access` | Yes      | Software access permissions | `RW`, `RO`, `WO`, `NA` |
| `hw_access` | Yes      | Hardware access permissions | `RW`, `RO`, `WO`, `NA` |

### 4.4 Read/Write Behavior Columns

|  Column   | Required |        Description         |                             Valid Values                              |
| --------- | -------- | -------------------------- | --------------------------------------------------------------------- |
| `onread`  | No       | Read side-effect behavior  | `rclr`, `rset`, `ruser`                                               |
| `onwrite` | No       | Write side-effect behavior | `woclr`, `woset`, `wot`, `wzs`, `wzc`, `wzt`, `wclr`, `wset`, `wuser` |

### 4.5 Optional Documentation Column

|    Column     | Required |                Description                |       Example        |
| ------------- | -------- | ----------------------------------------- | -------------------- |
| `description` | No       | Human-readable field/register description | `Enable control bit` |

### 4.6 Column Name Rules

All column headers MUST match the standard names EXACTLY and are case-sensitive:

- `addrmap_offset`
- `addrmap_name`
- `reg_offset`
- `reg_name`
- `reg_width`
- `field_name`
- `field_lsb`
- `field_msb`
- `reset_value`
- `sw_access`
- `hw_access`
- `onread` (optional)
- `onwrite` (optional)
- `description` (optional)

### 4.7 Array Support

Register arrays are specified directly in the `reg_name` column using SystemRDL syntax:

```csv
,,0x0000,BUFFER[8],32,,,,,,,8-element buffer array
```

This generates SystemRDL: `BUFFER[8] @ 0x0000` which expands to 8 registers with automatic address calculation. No additional columns needed.

---

## 5. Row Structure and Hierarchy

RCSV uses a **row-based approach** to define the three-level hierarchy: Address Map -> Register -> Field.

### 5.1 Row Type Identification

Rows are identified by which columns contain data:

|    Row Type     |                                Populated Columns                                |         Empty Columns         |
| --------------- | ------------------------------------------------------------------------------- | ----------------------------- |
| **Address Map** | `addrmap_offset`, `addrmap_name`                                                | All register/field columns    |
| **Register**    | `reg_offset`, `reg_name`, `reg_width`                                           | Address map and field columns |
| **Field**       | `field_name`, `field_lsb`, `field_msb`, `reset_value`, `sw_access`, `hw_access` | Address map columns           |

### 5.2 Structural Rules

1. **Header Row**: First row must contain column names
2. **Address Map Row**: Second row must define the address map
3. **Register Row**: Must appear before its associated field rows
4. **Field Rows**: Must immediately follow their parent register row
5. **Sequential Processing**: Rows processed in order, maintaining hierarchy

### 5.3 Example Structure

```csv
addrmap_offset,addrmap_name,reg_offset,reg_name,reg_width,field_name,field_lsb,field_msb,reset_value,sw_access,hw_access,description
0x0000,DEMO,,,,,,,,,,
,,0x0000,CTRL,32,,,,,,,"Control register"
,,,,,ENABLE,0,0,0,RW,RW,"Enable control bit"
,,,,,MODE,1,2,0,RW,RW,"Operation mode"
,,0x0004,STATUS,32,,,,,,,"Status register"
,,,,,READY,0,0,0,RO,RO,"Ready status"
```

**Row Type Explanation:**

- Row 2: Address Map Row (defines DEMO address map)
- Row 3: Register Row (defines CTRL register)
- Rows 4-5: Field Rows (ENABLE and MODE fields in CTRL register)
- Row 6: Register Row (defines STATUS register)
- Row 7: Field Row (READY field in STATUS register)

---

## 6. Access Control Semantics

RCSV uses separate `sw_access` and `hw_access` columns to specify field access permissions.

### 6.1 Software Access Values (`sw_access`)

| Value |    Meaning     | SystemRDL Equivalent |
| ----- | -------------- | -------------------- |
| `RW`  | Read/Write     | `sw = rw`            |
| `RO`  | Read Only      | `sw = r`             |
| `WO`  | Write Only     | `sw = w`             |
| `NA`  | Not Accessible | `sw = na`            |

### 6.2 Hardware Access Values (`hw_access`)

| Value |    Meaning     | SystemRDL Equivalent |
| ----- | -------------- | -------------------- |
| `RW`  | Read/Write     | `hw = rw`            |
| `RO`  | Read Only      | `hw = r`             |
| `WO`  | Write Only     | `hw = w`             |
| `NA`  | Not Accessible | `hw = na`            |

### 6.3 Common Access Patterns

| sw_access | hw_access |                Use Case                |
| --------- | --------- | -------------------------------------- |
| `RW`      | `RW`      | Control register                       |
| `RO`      | `WO`      | Status register (HW writes, SW reads)  |
| `WO`      | `RO`      | Command register (SW writes, HW reads) |
| `RO`      | `RO`      | Configuration constant                 |

---

## 7. Read/Write Behavior Semantics

RCSV supports SystemRDL's onread and onwrite behaviors for advanced field side-effects.

### 7.1 OnRead Behaviors (`onread`)

|  Value  |  Meaning   | SystemRDL Equivalent |             Use Case             |
| ------- | ---------- | -------------------- | -------------------------------- |
| `rclr`  | Read Clear | `onread = rclr`      | Status bits that clear when read |
| `rset`  | Read Set   | `onread = rset`      | Status bits that set when read   |
| `ruser` | Read User  | `onread = ruser`     | User-defined read behavior       |

### 7.2 OnWrite Behaviors (`onwrite`)

|  Value  |      Meaning      | SystemRDL Equivalent |          Use Case           |
| ------- | ----------------- | -------------------- | --------------------------- |
| `woclr` | Write One Clear   | `onwrite = woclr`    | Write 1 to clear (W1C)      |
| `woset` | Write One Set     | `onwrite = woset`    | Write 1 to set (W1S)        |
| `wot`   | Write One Toggle  | `onwrite = wot`      | Write 1 to toggle (W1T)     |
| `wzs`   | Write Zero Set    | `onwrite = wzs`      | Write 0 to set (W0S)        |
| `wzc`   | Write Zero Clear  | `onwrite = wzc`      | Write 0 to clear (W0C)      |
| `wzt`   | Write Zero Toggle | `onwrite = wzt`      | Write 0 to toggle (W0T)     |
| `wclr`  | Write Clear       | `onwrite = wclr`     | Any write clears            |
| `wset`  | Write Set         | `onwrite = wset`     | Any write sets              |
| `wuser` | Write User        | `onwrite = wuser`    | User-defined write behavior |

### 7.3 Common Read/Write Behavior Patterns

| sw_access | hw_access | onread | onwrite |                    Use Case                    |
| --------- | --------- | ------ | ------- | ---------------------------------------------- |
| `RO`      | `WO`      |        | `woclr` | Interrupt status (HW sets, SW clears with W1C) |
| `RW`      | `RW`      | `rclr` |         | Error counter (clears when read)               |
| `WO`      | `RO`      |        | `woset` | Command trigger (write 1 to execute)           |

---

## 8. Reset Value Semantics

The `reset_value` column specifies the field's initial value after reset.

### 8.1 Reset Value Format

- **Decimal**: `42`, `255`, `0`
- **Hexadecimal**: `0x2A`, `0xFF`, `0x0`
- **No Reset**: Leaving `reset_value` empty means "no reset property" (no `reset = ...` will be generated in SystemRDL)
- **Explicit Zero**: Setting `reset_value = 0` means an explicit reset of zero (`reset = 0`)

### 8.2 Reset Value Rules

1. **Field-Level**: Reset value applies to the field's bit range
2. **Right-Justified**: Value is aligned to field's LSB position
3. **Width Validation**: Value must fit within field width (MSB - LSB + 1)
4. **Register Composition**: Register reset = concatenation of all field resets

### 8.3 Examples

```csv
field_name,field_lsb,field_msb,reset_value,description
ENABLE,0,0,1,Enabled by default
MODE,1,3,5,Mode 5 (3-bit field: 0b101)
RESERVED,4,7,0,Reserved bits
```

---

## 9. Address and Size Semantics

RCSV uses explicit addressing without complex stride calculations.

### 9.1 Address Specification

- **`addrmap_offset`**: Base address of the address map (typically `0x0000`)
- **`reg_offset`**: Byte offset of register within the address map
- **Absolute Address**: `addrmap_offset + reg_offset`

### 9.2 Register Width

- **`reg_width`**: Register width in bits (typically 8, 16, 32, 64)
- **Byte Size**: Register size in bytes = `reg_width / 8`
- **Alignment**: Registers should be naturally aligned to their byte size

---

## 10. Validation Rules

RCSV enforces strict validation to ensure data integrity and SystemRDL compatibility.

### 10.1 Field Bit Range Validation

1. **Width Consistency**: Field width must match bit range calculation:
   - **Formula**: `field_width = field_msb - field_lsb + 1`
   - **Example**: Field `[7:4]` has width `7 - 4 + 1 = 4` bits
2. **Range Order**: `field_msb >= field_lsb` (MSB must be >= LSB)
3. **Register Bounds**: Field ranges must fit within register width (0 <= LSB <= MSB < reg_width)
4. **No Overlap**: Field bit ranges within a register must not overlap

### 10.2 Address Validation

1. **Hex/Decimal Format**: Addresses can be decimal (`4096`) or hex (`0x1000`)
2. **Alignment**: Register addresses SHOULD align to the register byte size. Parsers MAY warn on misalignment but SHOULD NOT fail.
3. **Uniqueness**: No duplicate register offsets within an address map

### 10.3 Access Control Validation

1. **Valid Values**: Only `RW`, `RO`, `WO`, `NA` allowed for access fields
2. **Case Insensitive**: `rw`, `RW`, `Rw` all accepted (normalized to uppercase)

### 10.4 Read/Write Behavior Validation

1. **OnRead Values**: Only `rclr`, `rset`, `ruser` allowed for onread fields
2. **OnWrite Values**: Only `woclr`, `woset`, `wot`, `wzs`, `wzc`, `wzt`, `wclr`, `wset`, `wuser` allowed for onwrite fields
3. **Case Insensitive**: Values are case-insensitive and normalized to lowercase

### 10.5 Reset Value Validation

1. **Width Check**: Reset value must fit in field width (< 2^width)
2. **Format Support**: Decimal and hexadecimal formats supported
3. **Negative Values**: Not supported (unsigned fields only)

### 10.6 Name Validation

1. **SystemRDL Identifiers**: Names must be valid SystemRDL identifiers (\[a-zA-Z\_\]\[a-zA-Z0-9\_\]*)
2. **No Reserved Words**: Cannot use SystemRDL keywords
3. **Uniqueness**: Field names must be unique within each register

### 10.7 Structural Validation

1. **Row Order**: Address map -> Register -> Fields sequence must be maintained
2. **Complete Hierarchy**: Every field must have a parent register
3. **Required Columns**: All mandatory columns must be present and non-empty
4. **Consistent Types**: Numeric columns must contain valid numbers

---

## 11. Minimal Compliance Set

For full RCSV compliance, all CSV files must include these columns:

**Required Columns (11 total):**

```csv
addrmap_offset, addrmap_name, reg_offset, reg_name, reg_width,
field_name, field_lsb, field_msb, reset_value, sw_access, hw_access
```

**Optional Columns:**

```csv
onread, onwrite, description
```

This minimal set ensures complete SystemRDL generation without information loss.

---

## 12. Complete Example

Here's a practical RCSV example demonstrating all features:

```csv
addrmap_offset,addrmap_name,reg_offset,reg_name,reg_width,field_name,field_lsb,field_msb,reset_value,sw_access,hw_access,onread,onwrite,description
0x0000,DEMO_CHIP,,,,,,,,,,,,"Demo chip register map"
,,0x0000,SYS_CTRL,32,,,,,,,,,"System control register"
,,,,,ENABLE,0,0,1,RW,RW,,,"System enable bit"
,,,,,MODE,1,3,2,RW,RW,,,"3-bit operation mode (0-7)"
,,,,,RESERVED_7_4,4,7,0,RO,NA,,,"Reserved bits"
,,,,,IRQ_EN,8,8,0,RW,RW,,,"Interrupt enable"
,,,,,DEBUG,9,9,0,RW,RW,,,"Debug mode enable"
,,,,,RESET_REQ,31,31,0,WO,RO,,"woset","Write 1 to trigger reset"
,,0x0004,STATUS,32,,,,,,,,,"Status register"
,,,,,READY,0,0,0,RO,WO,,,"System ready flag"
,,,,,ERROR,1,1,0,RO,WO,,"woclr","Error status (W1C)"
,,,,,INT_STATUS,8,15,0,RO,WO,"rclr",,"Interrupt status (clear on read)"
,,,,,DEVICE_ID,16,31,0xDEAD,RO,RO,,,"Device identification"
,,0x0008,DATA,32,,,,,,,,,"Data register"
,,,,,VALUE,0,31,0,RW,RW,,,"32-bit data value"
```

**Key Features Demonstrated:**

- Standard three-tier hierarchy (address map -> registers -> fields)
- Mixed access patterns (RW/RO/WO combinations)
- Hexadecimal values (addresses and reset values)
- Reserved field naming convention with recommended RO/NA access
- Complete bit coverage within registers
- Corrected field width descriptions (3-bit MODE field: 0-7)
- Read/write side-effects: Write-one-set (woset), Write-one-clear (woclr), Read-clear (rclr)

### 12.1 Array Example

Register arrays are specified directly in the `reg_name` column:

```csv
addrmap_offset,addrmap_name,reg_offset,reg_name,reg_width,field_name,field_lsb,field_msb,reset_value,sw_access,hw_access,description
0x0000,ARRAY_DEMO,,,,,,,,,,Array demonstration
,,0x0000,BUFFER[8],32,,,,,,,8-element buffer array
,,,,,DATA,0,31,0,RW,RW,Buffer data value
```

This generates SystemRDL `BUFFER[8] @ 0x0000` which expands to 8 registers: `BUFFER[0]` through `BUFFER[7]`.

---

## 13. SystemRDL Mapping

RCSV elements map directly to SystemRDL constructs:

### 13.1 Hierarchy Mapping

|             RCSV             |      SystemRDL Equivalent       |
| ---------------------------- | ------------------------------- |
| `addrmap_name`               | `addrmap <name> {`              |
| `reg_name` @ `reg_offset`    | `<reg_name> @ <reg_offset>;`    |
| `reg_name[N]` @ `reg_offset` | `<reg_name>[N] @ <reg_offset>;` |
| `field_name[msb:lsb]`        | `<field_name>[<msb>:<lsb>];`    |

### 13.2 Property Mapping

|         RCSV Column         |     SystemRDL Property      |
| --------------------------- | --------------------------- |
| `sw_access` = RW/RO/WO/NA   | `sw = rw/r/w/na`            |
| `hw_access` = RW/RO/WO/NA   | `hw = rw/r/w/na`            |
| `onread` = rclr/rset/ruser  | `onread = rclr/rset/ruser`  |
| `onwrite` = woclr/woset/... | `onwrite = woclr/woset/...` |
| `reset_value`               | `reset = <value>`           |
| `description`               | `desc = "<text>"`           |

### 13.3 Generated SystemRDL Example

From the RCSV example above, the generated SystemRDL would be:

```systemrdl
addrmap DEMO_CHIP @ 0x0000 {
    reg {
        field { sw = rw; hw = rw; reset = 1; desc = "System enable bit"; } ENABLE[0:0];
        field { sw = rw; hw = rw; reset = 2; desc = "3-bit operation mode (0-7)"; } MODE[3:1];
        field { sw = wo; hw = ro; onwrite = woset; desc = "Write 1 to trigger reset"; } RESET_REQ[31:31];
        // ... more fields
    } SYS_CTRL @ 0x0000;

    reg {
        field { sw = ro; hw = wo; desc = "System ready flag"; } READY[0:0];
        field { sw = ro; hw = wo; onwrite = woclr; desc = "Error status (W1C)"; } ERROR[1:1];
        field { sw = ro; hw = wo; onread = rclr; desc = "Interrupt status (clear on read)"; } INT_STATUS[15:8];
        // ... more fields
    } STATUS @ 0x0004;

    // ... more registers
};
```

---

## 14. Comments and Documentation

RCSV supports documentation through several mechanisms:

- **Description Column**: Use `description` column for field/register documentation
- **Multi-line Text**: Support for multi-line descriptions with CSV quoting
- **Reserved Fields**: Use descriptive names like `RESERVED_7_4` for gaps

---

## 15. Tool Compatibility and Migration

### 15.1 SystemRDL Toolkit Integration

RCSV is the **standard format** for the SystemRDL Toolkit's CSV2RDL converter:

- Command: `systemrdl_csv2rdl input.csv -o output.rdl`
- Validation: `python3 script/csv2rdl_validator.py`
- Testing: `make test-csv2rdl`

### 14.2 Migration from Legacy Formats

Existing CSV files can be migrated to RCSV compliance:

1. **Header Adjustment**: Rename columns to standard names
2. **Access Format**: Convert access values to RW/RO/WO/NA format
3. **Array Syntax**: Change separate array columns to reg_name[N] syntax
4. **Validation**: Run validation tools to verify compliance


---

## 15. Best Practices

### 15.1 Design Guidelines

1. **Complete Coverage**: Ensure all register bits are covered by fields (use RESERVED_X_Y for gaps)
2. **Consistent Naming**: Use clear, descriptive names following SystemRDL conventions
3. **Logical Grouping**: Group related registers by function or subsystem
4. **Address Alignment**: Align registers to natural boundaries (32-bit -> 4-byte alignment)
5. **Reserved Fields**: Use `RO/NA` access pattern for reserved bits (software read-only, hardware not accessible)

### 15.2 Documentation Standards

1. **Field Descriptions**: Provide clear, concise descriptions for all fields
2. **Multi-line Support**: Use CSV quoting for complex descriptions
3. **Reserved Fields**: Explicitly document reserved bit ranges
4. **Reset Values**: Always specify reset values, even if zero

### 15.3 Validation Workflow

1. **Format Check**: Verify CSV format and required columns
2. **Syntax Validation**: Run through CSV2RDL converter
3. **SystemRDL Parse**: Validate generated SystemRDL syntax
4. **Consistency Review**: Check field ranges, addresses, and access patterns

---

## 15. Troubleshooting

### 15.1 Common Errors

Field Overlap Error:

```bash
Error: Fields ENABLE[2:0] and MODE[1:3] overlap in register CTRL
Fix: Adjust field bit ranges to eliminate overlap
```

Invalid Access Value:

```bash
Error: Invalid sw_access value 'READ' (use RW/RO/WO/NA)
Fix: Use standard access control values
```

Address Alignment Warning:

```bash
Warning: Register at 0x0001 not aligned to 4-byte boundary
Fix: Use aligned addresses (0x0000, 0x0004, 0x0008, etc.)
```

### 15.2 Debugging Tips

1. **Use Validation Tools**: Run `script/csv2rdl_validator.py` for comprehensive checks
2. **Check Encoding**: Ensure UTF-8 encoding without BOM
3. **Verify Structure**: Confirm address map -> register -> field hierarchy
4. **Test Incremental**: Validate small sections before building complete maps

---

## 16. Summary

RCSV provides a **practical, standards-based approach** to CSV-SystemRDL conversion that:

- **Compatible** with existing SystemRDL Toolkit
- **Simple** to understand and create
- **Complete** - preserves all register information
- **Validated** - strict consistency checking
- **Documented** - comprehensive specification and examples

By following this specification, teams can create reliable, interchangeable register map definitions that integrate seamlessly with SystemRDL-based design flows.
