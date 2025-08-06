#include "SystemRDLLexer.h"
#include "SystemRDLParser.h"
#include "antlr4-runtime.h"
#include "cmdline_parser.h"
#include "elaborator.h"
#include "systemrdl_api.h"
#include "systemrdl_version.h"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace antlr4;
using namespace systemrdl;

// Printer for elaborated model
class ElaboratedModelPrinter : public ElaboratedModelTraverser
{
public:
    void print_model(ElaboratedAddrmap &root)
    {
        std::cout << "=== Elaborated SystemRDL Model ===" << std::endl;
        traverse(root);
    }

protected:
    void pre_visit(ElaboratedNode &node) override
    {
        // Print indentation
        for (int i = 0; i < depth_; i++) {
            std::cout << "  ";
        }

        // Print node information
        std::string icon = "[REG]";
        if (node.get_node_type() == "addrmap")
            icon = "[MAP]";
        else if (node.get_node_type() == "regfile")
            icon = "[FILE]";
        else if (node.get_node_type() == "reg")
            icon = "[REG]";
        else if (node.get_node_type() == "field")
            icon = "[FIELD]";
        else if (node.get_node_type() == "mem")
            icon = "[MEM]";

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
                if (i > 0)
                    std::cout << "x";
                std::cout << node.array_dimensions[i];
            }
            std::cout << "]";
        }

        std::cout << std::endl;

        // Print properties
        for (const auto &prop : node.properties) {
            for (int i = 0; i <= depth_; i++) {
                std::cout << "  ";
            }
            std::cout << "    " << prop.first << ": ";

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

    void post_visit(ElaboratedNode &node) override { depth_--; }

private:
    int depth_ = 0;
};

// Helper function to generate default JSON filename
std::string get_default_ast_filename(const std::string &input_file, const std::string &suffix = "")
{
    // Simple basename extraction
    size_t last_slash = input_file.find_last_of("/\\");
    size_t last_dot   = input_file.find_last_of('.');

    std::string basename;
    if (last_slash != std::string::npos) {
        basename = input_file.substr(last_slash + 1);
    } else {
        basename = input_file;
    }

    if (last_dot != std::string::npos && last_dot > last_slash) {
        size_t trim_pos = last_dot - (last_slash != std::string::npos ? last_slash + 1 : 0);
        basename.resize(trim_pos);
    }

    return basename + suffix + ".json";
}

int main(int argc, char *argv[])
{
    // Setup command line parser
    CmdLineParser cmdline("SystemRDL Elaborator - Parse and elaborate SystemRDL files");
    cmdline.set_version(systemrdl::get_detailed_version());
    cmdline.add_option_with_optional_value(
        "a", "ast", "Enable AST JSON output, optionally specify filename");
    cmdline.add_option_with_optional_value(
        "j", "json", "Enable simplified JSON output, optionally specify filename");
    cmdline.add_option("h", "help", "Show this help message");

    if (!cmdline.parse(argc, argv)) {
        return argc == 2
                       && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"
                           || std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v")
                   ? 0
                   : 1;
    }

    const auto &args = cmdline.get_positional_args();
    if (args.empty()) {
        std::cerr << "Error: No input file specified" << std::endl;
        cmdline.print_help();
        return 1;
    }

    std::string inputFile = args[0];

    try {
        // 1. Parsing phase
        std::cout << "[PARSE] Parsing SystemRDL file: " << inputFile << std::endl;

        std::ifstream stream(inputFile);
        if (!stream.is_open()) {
            std::cerr << "Error: Cannot open file " << inputFile << std::endl;
            return 1;
        }

        ANTLRInputStream  input(stream);
        SystemRDLLexer    lexer(&input);
        CommonTokenStream tokens(&lexer);
        SystemRDLParser   parser(&tokens);

        tree::ParseTree *tree = parser.root();

        if (parser.getNumberOfSyntaxErrors() > 0) {
            std::cerr << "Syntax errors found: " << parser.getNumberOfSyntaxErrors() << std::endl;
            return 1;
        }

        std::cout << "[OK] Parsing successful!" << std::endl;

        // 2. Elaboration phase
        std::cout << "\n[ELAB] Starting elaboration..." << std::endl;

        SystemRDLElaborator elaborator;
        auto                root_context     = dynamic_cast<SystemRDLParser::RootContext *>(tree);
        auto                elaborated_model = elaborator.elaborate(root_context);

        if (elaborator.has_errors()) {
            std::cerr << "Elaboration errors:" << std::endl;
            for (const auto &error : elaborator.get_errors()) {
                std::cerr << "  Line " << error.line << ":" << error.column << " - "
                          << error.message << std::endl;
            }
            return 1;
        }

        if (!elaborated_model) {
            std::cerr << "Failed to elaborate model" << std::endl;
            return 1;
        }

        std::cout << "[OK] Elaboration successful!" << std::endl;

        // 3. Print elaborated model
        std::cout << "\n" << std::string(50, '=') << std::endl;
        ElaboratedModelPrinter printer;
        printer.print_model(*elaborated_model);

        // 4. Generate address mapping
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "[ADDR] Address Map:" << std::endl;
        std::cout << std::string(50, '=') << std::endl;

        AddressMapGenerator addr_gen;
        auto                address_map = addr_gen.generate_address_map(*elaborated_model);

        std::cout << std::left << std::setw(12) << "Address" << std::setw(8) << "Size"
                  << std::setw(20) << "Name" << "Path" << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        for (const auto &entry : address_map) {
            printf(
                "0x%08lx  %-6lu  %-18s  %s\n",
                entry.address,
                entry.size,
                entry.name.c_str(),
                entry.path.c_str());
        }

        // 5. Generate AST JSON output if requested
        if (cmdline.is_set("ast")) {
            std::string output_file = cmdline.get_value("ast");

            // If no filename provided, generate default
            if (output_file.empty()) {
                output_file = get_default_ast_filename(inputFile, "_ast_elaborated");
            }

            std::cout << "\nGenerating AST JSON output..." << std::endl;

            // Use unified API for consistent JSON output
            systemrdl::Result result = systemrdl::file::elaborate(inputFile);
            if (result.ok()) {
                std::ofstream outFile(output_file);
                if (outFile.is_open()) {
                    outFile << result.value();
                    outFile.close();
                    std::cout << "AST JSON output written to: " << output_file << std::endl;
                } else {
                    std::cerr << "Failed to write AST JSON output to: " << output_file << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Failed to generate AST JSON: " << result.error() << std::endl;
                return 1;
            }
        }

        // 6. Generate simplified JSON output if requested
        if (cmdline.is_set("json")) {
            std::string output_file = cmdline.get_value("json");

            // If no filename provided, generate default
            if (output_file.empty()) {
                output_file = get_default_ast_filename(inputFile, "_simplified");
            }

            std::cout << "\nGenerating simplified JSON output..." << std::endl;

            // Use unified API for consistent JSON output
            systemrdl::Result result = systemrdl::file::elaborate_simplified(inputFile);
            if (result.ok()) {
                std::ofstream outFile(output_file);
                if (outFile.is_open()) {
                    outFile << result.value();
                    outFile.close();
                    std::cout << "Simplified JSON output written to: " << output_file << std::endl;
                } else {
                    std::cerr << "Failed to write simplified JSON output to: " << output_file
                              << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Failed to generate simplified JSON: " << result.error() << std::endl;
                return 1;
            }
        }

        std::cout << "\nElaboration completed successfully!" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
