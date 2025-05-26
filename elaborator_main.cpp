#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include "antlr4-runtime.h"
#include "SystemRDLLexer.h"
#include "SystemRDLParser.h"
#include "elaborator.h"

using namespace antlr4;
using namespace systemrdl;

// Printer for elaborated model
class ElaboratedModelPrinter : public ElaboratedModelTraverser {
public:
    void print_model(ElaboratedAddrmap& root) {
        std::cout << "=== Elaborated SystemRDL Model ===" << std::endl;
        traverse(root);
    }

protected:
    void pre_visit(ElaboratedNode& node) override {
        // Print indentation
        for (int i = 0; i < depth_; i++) {
            std::cout << "  ";
        }

        // Print node information
        std::string icon = "🔧";
        if (node.get_node_type() == "addrmap") icon = "📦";
        else if (node.get_node_type() == "regfile") icon = "📁";
        else if (node.get_node_type() == "reg") icon = "🔧";
        else if (node.get_node_type() == "field") icon = "🔧";
        else if (node.get_node_type() == "mem") icon = "💾";

        std::cout << icon << " " << node.get_node_type() << ": " << node.inst_name;

        if (node.absolute_address != 0 || node.get_node_type() == "addrmap") {
            std::cout << " @ 0x" << std::hex << node.absolute_address << std::dec;
        }

        // For fields, show bit range
        if (node.get_node_type() == "field") {
            auto msb_prop = node.get_property("msb");
            auto lsb_prop = node.get_property("lsb");
            if (msb_prop && lsb_prop) {
                std::cout << " [" << msb_prop->int_val << ":" << lsb_prop->int_val << "]";
            }
        }

        if (node.size > 0) {
            std::cout << " (size: " << node.size << " bytes)";
        }

        if (!node.array_dimensions.empty()) {
            std::cout << " [array: ";
            for (size_t i = 0; i < node.array_dimensions.size(); ++i) {
                if (i > 0) std::cout << "x";
                std::cout << node.array_dimensions[i];
            }
            std::cout << "]";
        }

        std::cout << std::endl;

        // Print properties
        for (const auto& prop : node.properties) {
            for (int i = 0; i <= depth_; i++) {
                std::cout << "  ";
            }
            std::cout << "📝 " << prop.first << ": ";

            switch (prop.second.type) {
                case PropertyValue::STRING:
                    std::cout << "\"" << prop.second.string_val << "\"";
                    break;
                case PropertyValue::INTEGER:
                    std::cout << prop.second.int_val;
                    break;
                case PropertyValue::BOOLEAN:
                    std::cout << (prop.second.bool_val ? "true" : "false");
                    break;
                default:
                    std::cout << "unknown";
            }
            std::cout << std::endl;
        }

        depth_++;
    }

    void post_visit(ElaboratedNode& node) override {
        depth_--;
    }

private:
    int depth_ = 0;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file.rdl>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];

    try {
        // 1. Parsing phase
        std::cout << "🔧 Parsing SystemRDL file: " << inputFile << std::endl;

        std::ifstream stream(inputFile);
        if (!stream.is_open()) {
            std::cerr << "Error: Cannot open file " << inputFile << std::endl;
            return 1;
        }

        ANTLRInputStream input(stream);
        SystemRDLLexer lexer(&input);
        CommonTokenStream tokens(&lexer);
        SystemRDLParser parser(&tokens);

        tree::ParseTree* tree = parser.root();

        if (parser.getNumberOfSyntaxErrors() > 0) {
            std::cerr << "Syntax errors found: " << parser.getNumberOfSyntaxErrors() << std::endl;
            return 1;
        }

        std::cout << "✅ Parsing successful!" << std::endl;

        // 2. Elaboration phase
        std::cout << "\n🚀 Starting elaboration..." << std::endl;

        SystemRDLElaborator elaborator;
        auto root_context = dynamic_cast<SystemRDLParser::RootContext*>(tree);
        auto elaborated_model = elaborator.elaborate(root_context);

        if (elaborator.has_errors()) {
            std::cerr << "Elaboration errors:" << std::endl;
            for (const auto& error : elaborator.get_errors()) {
                std::cerr << "  Line " << error.line << ":" << error.column
                         << " - " << error.message << std::endl;
            }
            return 1;
        }

        if (!elaborated_model) {
            std::cerr << "Failed to elaborate model" << std::endl;
            return 1;
        }

        std::cout << "✅ Elaboration successful!" << std::endl;

        // 3. Print elaborated model
        std::cout << "\n" << std::string(50, '=') << std::endl;
        ElaboratedModelPrinter printer;
        printer.print_model(*elaborated_model);

        // 4. Generate address mapping
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "📊 Address Map:" << std::endl;
        std::cout << std::string(50, '=') << std::endl;

        AddressMapGenerator addr_gen;
        auto address_map = addr_gen.generate_address_map(*elaborated_model);

        std::cout << std::left << std::setw(12) << "Address"
                  << std::setw(8) << "Size"
                  << std::setw(20) << "Name"
                  << "Path" << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        for (const auto& entry : address_map) {
            printf("0x%08lx  %-6lu  %-18s  %s\n",
                   entry.address, entry.size, entry.name.c_str(), entry.path.c_str());
        }

        std::cout << "\n🎉 Elaboration completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}