# SystemRDL Library Example

This directory contains a complete working example showing how to use the SystemRDL library in your own C++ projects.

## Prerequisites

First, you need to build and install the SystemRDL library. From the project root directory:

```bash
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install
```

## Building the Example

Once the library is installed, build this example:

```bash
cd example
mkdir build && cd build
cmake ..
make
```

## Running the Example

Run the example with the provided test RDL file:

```bash
./example_app ../test_example.rdl
```

Or use any of the test files from the main project:

```bash
./example_app ../../test/test_basic_chip.rdl
```

## What the Example Does

The example demonstrates:

- Parsing SystemRDL files using the library
- Elaborating the parsed design
- Error handling
- Extracting design information
- Working with elaborated nodes

See `example.cpp` for the complete source code and comments explaining each step.
