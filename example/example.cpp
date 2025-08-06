#include "systemrdl_api.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

int main()
{
    std::cout << "[API] SystemRDL Modern API Example\n" << std::endl;

    // Example 1: Parse SystemRDL string content
    {
        std::cout << "[1] Example 1: Parse SystemRDL content" << std::endl;

        std::string rdl_content = R"(
            addrmap simple_chip {
                reg {
                    field {
                        sw = rw;
                        hw = r;
                        desc = "Control bit";
                    } ctrl[0:0] = 0;

                    field {
                        sw = rw;
                        hw = r;
                        desc = "Status bits";
                    } status[7:4] = 0;
                } control_reg @ 0x0000;
            };
        )";

        auto result = systemrdl::parse(rdl_content);
        if (result.ok()) {
            std::cout << "[OK] Parse successful!" << std::endl;
            std::cout << "[OUT] AST JSON (first 200 chars): " << result.value().substr(0, 200)
                      << "..." << std::endl;
        } else {
            std::cout << "[ERR] Parse failed: " << result.error() << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 2: Full AST JSON Elaboration
    {
        std::cout << "[2] Example 2: Full AST JSON Elaboration" << std::endl;

        std::string rdl_content = R"(
            addrmap demo_chip {
                name = "Demo Chip";
                desc = "Demonstration chip for elaboration";

                reg {
                    name = "Control Register";
                    regwidth = 32;

                    field {
                        name = "ENABLE";
                        desc = "Enable control";
                        sw = rw;
                        hw = r;
                    } enable[0:0] = 0;

                    field {
                        name = "MODE";
                        desc = "Operation mode";
                        sw = rw;
                        hw = r;
                    } mode[3:1] = 0;
                } ctrl_reg @ 0x0000;

                reg {
                    name = "Status Register";
                    regwidth = 32;

                    field {
                        name = "READY";
                        desc = "System ready";
                        sw = r;
                        hw = w;
                    } ready[0:0] = 0;

                    field {
                        name = "COUNT";
                        desc = "Status counter";
                        sw = r;
                        hw = w;
                    } count[15:8] = 0;
                } status_reg @ 0x0004;
            };
        )";

        auto result = systemrdl::elaborate(rdl_content);
        if (result.ok()) {
            std::cout << "[OK] Elaboration successful!" << std::endl;
            std::cout << "[OUT] Elaborated JSON (first 300 chars): "
                      << result.value().substr(0, 300) << "..." << std::endl;

            // Count the number of nodes in the elaborated model
            std::string json       = result.value();
            size_t      node_count = 0;
            size_t      pos        = 0;
            while ((pos = json.find("\"node_type\":", pos)) != std::string::npos) {
                node_count++;
                pos++;
            }
            std::cout << "[INFO] Total elaborated nodes: " << node_count << std::endl;
        } else {
            std::cout << "[ERR] Elaboration failed: " << result.error() << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 3: Simplified JSON Elaboration (Recommended for Templates)
    {
        std::cout << "[3] Example 3: Simplified JSON Elaboration (Recommended for Templates)"
                  << std::endl;

        std::string rdl_content = R"(
            addrmap demo_chip {
                name = "Demo Chip";
                desc = "Demonstration chip for simplified JSON";

                reg {
                    name = "Control Register";
                    regwidth = 32;

                    field {
                        name = "ENABLE";
                        desc = "Enable control";
                        sw = rw;
                        hw = r;
                    } enable[0:0] = 0;

                    field {
                        name = "MODE";
                        desc = "Operation mode";
                        sw = rw;
                        hw = r;
                    } mode[3:1] = 0;
                } ctrl_reg @ 0x0000;

                reg {
                    name = "Status Register";
                    regwidth = 32;

                    field {
                        name = "READY";
                        desc = "System ready";
                        sw = r;
                        hw = w;
                    } ready[0:0] = 0;
                } status_reg @ 0x0004;
            };
        )";

        auto result = systemrdl::elaborate_simplified(rdl_content);
        if (result.ok()) {
            std::cout << "[OK] Simplified elaboration successful!" << std::endl;
            std::cout << "[OUT] Simplified JSON (first 400 chars): "
                      << result.value().substr(0, 400) << "..." << std::endl;

            // Check for simplified JSON characteristics
            std::string json       = result.value();
            bool        has_format = json.find("\"format\"") != std::string::npos;
            bool has_simplified    = json.find("SystemRDL_SimplifiedModel") != std::string::npos;
            bool has_registers     = json.find("\"registers\"") != std::string::npos;

            std::cout << "[INFO] Simplified JSON validation:" << std::endl;
            std::cout << "  - Has format field: " << (has_format ? "✓" : "✗") << std::endl;
            std::cout << "  - Is simplified model: " << (has_simplified ? "✓" : "✗") << std::endl;
            std::cout << "  - Has registers array: " << (has_registers ? "✓" : "✗") << std::endl;
        } else {
            std::cout << "[ERR] Simplified elaboration failed: " << result.error() << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 4: Advanced Elaboration with Arrays
    {
        std::cout << "[4] Example 4: Advanced Elaboration (Arrays & Complex Features)" << std::endl;

        std::string complex_rdl = R"(
            addrmap advanced_soc {
                name = "Advanced SoC";
                desc = "Complex SoC with multiple components";

                regfile {
                    name = "CPU Control Block";
                    desc = "CPU configuration registers";

                    reg {
                        name = "CPU Control";
                        regwidth = 32;

                        field {
                            name = "CPU_ENABLE";
                            desc = "CPU core enable";
                            sw = rw;
                            hw = r;
                        } cpu_en[0:0] = 0;

                        field {
                            name = "CLOCK_DIV";
                            desc = "Clock divider";
                            sw = rw;
                            hw = r;
                        } clk_div[7:4] = 1;
                    } cpu_ctrl @ 0x00;
                } cpu_block @ 0x0000;

                reg {
                    name = "Memory Controller";
                    regwidth = 32;

                    field {
                        name = "MEM_ENABLE";
                        desc = "Memory controller enable";
                        sw = rw;
                        hw = r;
                    } mem_en[0:0] = 0;

                    field {
                        name = "REFRESH_RATE";
                        desc = "Memory refresh rate";
                        sw = rw;
                        hw = r;
                    } refresh[15:8] = 0x80;
                } mem_ctrl[4] @ 0x1000 += 0x100;
            };
        )";

        auto result = systemrdl::elaborate(complex_rdl);
        if (result.ok()) {
            std::cout << "[OK] Advanced elaboration successful!" << std::endl;

            // Count different types of nodes
            std::string json          = result.value();
            size_t      addrmap_count = 0, regfile_count = 0, reg_count = 0, field_count = 0;
            size_t      pos = 0;

            while ((pos = json.find("\"node_type\":", pos)) != std::string::npos) {
                size_t start = pos + 13; // length of "node_type":
                size_t end   = json.find(",", start);
                if (end == std::string::npos)
                    end = json.find("}", start);

                std::string node_type = json.substr(start, end - start);
                // Remove quotes and whitespace
                node_type
                    .erase(std::remove(node_type.begin(), node_type.end(), '"'), node_type.end());
                node_type
                    .erase(std::remove(node_type.begin(), node_type.end(), ' '), node_type.end());

                if (node_type == "addrmap")
                    addrmap_count++;
                else if (node_type == "regfile")
                    regfile_count++;
                else if (node_type == "reg")
                    reg_count++;
                else if (node_type == "field")
                    field_count++;

                pos++;
            }

            std::cout << "[INFO] Elaborated Structure:" << std::endl;
            std::cout << "   [MAP] Address Maps: " << addrmap_count << std::endl;
            std::cout << "   [FILE] Register Files: " << regfile_count << std::endl;
            std::cout << "   [REG] Registers: " << reg_count << std::endl;
            std::cout << "   [FIELD] Fields: " << field_count << std::endl;
            std::cout << "   [TOTAL] Total Nodes: "
                      << (addrmap_count + regfile_count + reg_count + field_count) << std::endl;

            // Show size of elaborated JSON
            std::cout << "[INFO] Elaborated JSON size: " << json.length() << " bytes" << std::endl;
            std::cout << "[DEMO] This demonstrates:" << std::endl;
            std::cout << "   - Array instantiation (mem_ctrl[4])" << std::endl;
            std::cout << "   - Complex address mapping with strides" << std::endl;
            std::cout << "   - Hierarchical regfile structures" << std::endl;
            std::cout << "   - Automatic gap filling and validation" << std::endl;
            std::cout << "   - Property inheritance and elaboration" << std::endl;
        } else {
            std::cout << "[ERR] Advanced elaboration failed: " << result.error() << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 5: CSV to SystemRDL conversion
    {
        std::cout << "[5] Example 5: Convert CSV to SystemRDL" << std::endl;

        std::string csv_content
            = "addrmap_offset,addrmap_name,reg_offset,reg_name,reg_width,field_name,field_lsb,"
              "field_msb,reset_value,sw_access,hw_access,description\n"
              "0x0000,DEMO,,,,,,,,,,\n"
              ",,0x0000,CTRL,32,,,,,,,Control register\n"
              ",,,,,ENABLE,0,0,0,RW,RW,Enable control bit\n"
              ",,,,,MODE,1,2,0,RW,RW,Operation mode\n"
              ",,0x0004,STATUS,32,,,,,,,Status register\n"
              ",,,,,READY,0,0,0,RO,RO,Ready status\n"
              ",,,,,ERROR,1,1,0,RO,RO,Error flag\n";

        auto result = systemrdl::csv_to_rdl(csv_content);
        if (result.ok()) {
            std::cout << "[OK] CSV conversion successful!" << std::endl;
            std::cout << "[OUT] SystemRDL output:\n" << result.value() << std::endl;
        } else {
            std::cout << "[ERR] CSV conversion failed: " << result.error() << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 5: File-based operations
    {
        std::cout << "[5] Example 5: File-based operations" << std::endl;

        // Create a test file
        std::ofstream test_file("test_example.rdl");
        test_file << R"(
            addrmap file_test {
                reg {
                    field {
                        sw = rw;
                    } test_field[15:0];
                } test_reg @ 0x0;
            };
        )";
        test_file.close();

        // Parse file
        auto parse_result = systemrdl::file::parse("test_example.rdl");
        if (parse_result.ok()) {
            std::cout << "[OK] File parse successful!" << std::endl;
            std::cout << "[OUT] File AST JSON (first 200 chars): "
                      << parse_result.value().substr(0, 200) << "..." << std::endl;
        } else {
            std::cout << "[ERR] File parse failed: " << parse_result.error() << std::endl;
        }

        // Elaborate file
        auto elaborate_result = systemrdl::file::elaborate("test_example.rdl");
        if (elaborate_result.ok()) {
            std::cout << "[OK] File elaboration successful!" << std::endl;
            std::cout << "[OUT] File elaborated JSON (first 200 chars): "
                      << elaborate_result.value().substr(0, 200) << "..." << std::endl;
        } else {
            std::cout << "[ERR] File elaboration failed: " << elaborate_result.error() << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 6: Stream operations
    {
        std::cout << "[6] Example 6: Stream operations" << std::endl;

        std::string rdl_content = R"(
            addrmap stream_test {
                reg {
                    field {
                        sw = rw;
                    } stream_field[7:0];
                } stream_reg @ 0x0;
            };
        )";

        std::istringstream input(rdl_content);
        std::ostringstream output;

        if (systemrdl::stream::parse(input, output)) {
            std::cout << "[OK] Stream parse successful!" << std::endl;
            std::cout << "[OUT] Stream output (first 200 chars): " << output.str().substr(0, 200)
                      << "..." << std::endl;
        } else {
            std::cout << "[ERR] Stream parse failed!" << std::endl;
        }

        // Test stream elaboration
        std::istringstream elab_input(rdl_content);
        std::ostringstream elab_output;

        if (systemrdl::stream::elaborate(elab_input, elab_output)) {
            std::cout << "[OK] Stream elaboration successful!" << std::endl;
            std::cout << "[OUT] Stream elaborated output (first 200 chars): "
                      << elab_output.str().substr(0, 200) << "..." << std::endl;
        } else {
            std::cout << "[ERR] Stream elaboration failed!" << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 7: Error handling demonstration
    {
        std::cout << "[7] Example 7: Error handling" << std::endl;

        std::string invalid_rdl = "invalid SystemRDL syntax here!!!";

        auto result = systemrdl::parse(invalid_rdl);
        if (!result.ok()) {
            std::cout << "[OK] Error handling working correctly!" << std::endl;
            std::cout << "[ERR] Error message: " << result.error() << std::endl;
        } else {
            std::cout << "[ERR] Expected error but got success!" << std::endl;
        }

        // Test elaboration error handling
        auto elab_result = systemrdl::elaborate(invalid_rdl);
        if (!elab_result.ok()) {
            std::cout << "[OK] Elaboration error handling working correctly!" << std::endl;
            std::cout << "[ERR] Elaboration error: " << elab_result.error() << std::endl;
        } else {
            std::cout << "[ERR] Expected elaboration error but got success!" << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "[OK] SystemRDL Modern API example completed." << std::endl;
    std::cout << "\n[INFO] Key features of the API:" << std::endl;
    std::cout << "   - Clean interface without ANTLR4 header exposure" << std::endl;
    std::cout << "   - String-based input/output for ease of use" << std::endl;
    std::cout << "   - Consistent error handling pattern" << std::endl;
    std::cout << "   - Multiple input/output methods supported" << std::endl;
    std::cout << "   - Modern C++ design patterns" << std::endl;
    std::cout << "   - Elaboration functionality available" << std::endl;
    std::cout << "\n[INFO] Elaboration capabilities demonstrated:" << std::endl;
    std::cout << "   - Hierarchical design processing" << std::endl;
    std::cout << "   - Array and parameterization support" << std::endl;
    std::cout << "   - Address calculation assistance" << std::endl;
    std::cout << "   - Basic validation features" << std::endl;
    std::cout << "   - Property inheritance handling" << std::endl;
    std::cout << "   - Memory management through RAII" << std::endl;
    std::cout << "\n[INFO] This example shows the basic usage patterns of the toolkit."
              << std::endl;

    return 0;
}
