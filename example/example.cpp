#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <iomanip>

// Include SystemRDL headers
#include <systemrdl/elaborator.h>
#include <systemrdl/SystemRDLLexer.h>
#include <systemrdl/SystemRDLParser.h>

// ANTLR4 includes
#include <antlr4-runtime.h>

using namespace antlr4;
using namespace systemrdl;

int main(int argc, char* argv[])
{
    std::cout << "SystemRDL Library Example\n";
    std::cout << "========================\n\n";

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <rdl_file>\n";
        std::cout << "Example: " << argv[0] << " test.rdl\n";
        return 1;
    }

    std::string rdl_file = argv[1];
    std::cout << "Processing SystemRDL file: " << rdl_file << "\n\n";

    try {
        // Step 1: Read and parse the SystemRDL file
        std::ifstream stream(rdl_file);
        if (!stream.is_open()) {
            std::cerr << "Error: Cannot open file " << rdl_file << std::endl;
            return 1;
        }

        ANTLRInputStream input(stream);
        SystemRDLLexer lexer(&input);
        CommonTokenStream tokens(&lexer);
        SystemRDLParser parser(&tokens);

        // Parse the file
        auto tree = parser.root();

        std::cout << "✓ Parsing completed successfully\n";

        // Step 2: Create elaborator and process the AST
        Elaborator elaborator;
        auto root_node = elaborator.elaborate(tree);

        if (elaborator.has_errors()) {
            std::cout << "\n❌ Elaboration errors found:\n";
            for (const auto& error : elaborator.get_errors()) {
                std::cout << "  Error at line " << error.line
                         << ", column " << error.column
                         << ": " << error.message << "\n";
            }
            return 1;
        }

        std::cout << "✓ Elaboration completed successfully\n";

        // Step 3: Display information about the elaborated design
        if (root_node) {
            std::cout << "\n📋 Design Information:\n";
            std::cout << "  Root node type: " << root_node->get_node_type() << "\n";
            std::cout << "  Instance name: " << root_node->inst_name << "\n";
            std::cout << "  Type name: " << root_node->type_name << "\n";
            std::cout << "  Base address: 0x" << std::hex << root_node->absolute_address << std::dec << "\n";
            std::cout << "  Size: " << root_node->size << " bytes\n";
            std::cout << "  Children count: " << root_node->children.size() << "\n";

            // Display some properties if they exist
            if (!root_node->properties.empty()) {
                std::cout << "\n📝 Properties:\n";
                for (const auto& prop_pair : root_node->properties) {
                    const std::string& prop_name = prop_pair.first;
                    const PropertyValue& prop_value = prop_pair.second;
                    std::cout << "  " << prop_name << " = ";
                    switch (prop_value.type) {
                        case PropertyValue::STRING:
                            std::cout << "\"" << prop_value.string_val << "\"";
                            break;
                        case PropertyValue::INTEGER:
                            std::cout << prop_value.int_val;
                            break;
                        case PropertyValue::BOOLEAN:
                            std::cout << (prop_value.bool_val ? "true" : "false");
                            break;
                        case PropertyValue::ENUM:
                            std::cout << prop_value.string_val << " (enum)";
                            break;
                    }
                    std::cout << "\n";
                }
            }

            // Create and use address map visitor to display address layout
            AddressMapVisitor addr_visitor;
            root_node->accept_visitor(addr_visitor);

            const auto& addr_map = addr_visitor.get_address_map();
            if (!addr_map.empty()) {
                std::cout << "\n🗺️  Address Map:\n";
                std::cout << "  Address       Size      Name                 Path\n";
                std::cout << "  --------      ----      ----                 ----\n";
                for (const auto& entry : addr_map) {
                    std::cout << "  0x" << std::hex << std::setw(8) << std::setfill('0')
                             << entry.address << std::dec << "  "
                             << std::setw(8) << entry.size << "  "
                             << std::setw(20) << std::left << entry.name
                             << entry.path << "\n";
                }
            }
        }

        std::cout << "\n✅ Processing completed successfully!\n";

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}