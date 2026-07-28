# Architecture

How a register description becomes JSON, and the rules the code holds itself to.
For where files live, read the tree in [CONTRIBUTING.md](../CONTRIBUTING.md).

## Pipeline

```text
.rdl text
   |  ANTLR4, from grammar/SystemRDL.g4
   v
parse tree
   |  SystemRDLElaborator, src/elaborator.cpp
   v
ElaboratedNode tree     addrmap / regfile / reg / field / mem
   |  src/systemrdl_api.cpp
   v
JSON
```

`systemrdl_api.cpp` also holds the RCSV reader, which produces `.rdl` text and
then re-enters the same pipeline. There is no second parser.

## Elaborating a register

Order matters here, and each step depends on the one before it.

1. Validate the register's own properties. A bad width makes everything below
   meaningless, so this runs first.
2. Settle the bit ordering. `[low:high]` anywhere selects msb0, which decides
   the direction field packing runs in.
3. Position the fields that have no explicit range.
4. Validate field ranges and overlaps.
5. Fill the gaps between fields with reserved fields.
6. Compute the size.
7. Validate the reset values, before the next step clips them.
8. Build the register reset image from the field values.

Step 7 before step 8 is not cosmetic: building the image pins each reset value
to its field width, after which an oversized value can no longer be detected.

## Placing an instance

An address written with `@` is taken as given. Otherwise the instance is placed
after its predecessor and then aligned, which can only happen once its size is
known, which is only after its body has been elaborated. Its children have taken
their addresses by then, so `place_instance` moves the whole subtree.

## Rules the code keeps

**Numbers enter through one door.** `parse_number` in `src/systemrdl_number.cpp`
is the only way literal text becomes a value. `std::stoll` and `std::stoull`
stop at the apostrophe of `8'hFF` and at the separator of `0x1_0000_0000` and
return the prefix, which is how a register description silently acquires the
wrong reset value.

**A field value is a bit vector, not an integer.** `BitVector` is fixed width,
so a 256-bit reset value and a 1-bit one take the same code path and there is no
64-bit boundary to special-case.

**Bit positions are normalised.** `msb` and `lsb` on a field always satisfy
`msb >= lsb`, whichever ordering the source used. Only the register remembers
which form was written.

**Parameters nest.** Scopes are a stack, because a parameterised register
instantiated inside a parameterised regfile must still see the outer
parameters.

**Nothing is guessed.** A dimension that will not evaluate, a memory with no
width, a register with no field: each is an error naming the clause it breaks.
A plausible default is worse than a refusal, because it reaches RTL.

## Where behaviour is decided

| Question | File |
| -- | -- |
| What is valid SystemRDL syntax | `grammar/SystemRDL.g4` |
| What a literal means | `src/systemrdl_number.cpp` |
| Where fields and instances land | `src/elaborator.cpp` |
| What the JSON looks like | `src/systemrdl_api.cpp` |
| What the standard requires | `test/test_spec_*.rdl` |
