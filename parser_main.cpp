#include "SystemRDLLexer.h"
#include "SystemRDLParser.h"
#include "antlr4-runtime.h"
#include "cmdline_parser.h"
#include "systemrdl_api.h"
#include "systemrdl_version.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace antlr4;

// Recursive function to print AST (optimized alignment version)
void printAST(tree::ParseTree *tree, SystemRDLParser *parser, int depth = 0)
{
    if (ParserRuleContext *ruleContext = dynamic_cast<ParserRuleContext *>(tree)) {
        std::string ruleName = parser->getRuleNames()[ruleContext->getRuleIndex()];
        std::string text     = ruleContext->getText();

        // Print indentation
        auto indent = [depth]() {
            for (int i = 0; i < depth; i++)
                std::cout << "  ";
        };

        // Function to handle multi-line text alignment
        auto printAligned = [&](const std::string &prefix, const std::string &content) {
            indent();
            std::cout << prefix;

            // Calculate alignment position (indentation + prefix length)
            size_t      alignPos = depth * 2 + prefix.length();
            std::string alignSpaces(alignPos, ' ');

            // Split text into lines
            std::istringstream iss(content);
            std::string        line;
            bool               firstLine = true;

            while (std::getline(iss, line)) {
                if (firstLine) {
                    std::cout << line << std::endl;
                    firstLine = false;
                } else {
                    std::cout << alignSpaces << line << std::endl;
                }
            }
        };

        // Different processing based on rule type
        if (ruleName == "component_named_def") {
            indent();
            std::cout << "[COMP] Component Definition" << std::endl;
        } else if (ruleName == "component_type_primary") {
            printAligned("[TYPE] ", text);
        } else if (ruleName == "component_inst") {
            printAligned("[INST] ", text);
        } else if (ruleName == "local_property_assignment") {
            printAligned("[PROP] ", text);
        } else if (ruleName == "range_suffix") {
            printAligned("[RANGE] ", text);
        } else if (ruleName == "inst_addr_fixed") {
            printAligned("[ADDR] ", text);
        }

        // Recursively process child nodes
        for (size_t i = 0; i < ruleContext->children.size(); i++) {
            printAST(ruleContext->children[i], parser, depth + 1);
        }
    }
}

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
    CmdLineParser cmdline("SystemRDL Parser - Parse SystemRDL files and display AST");
    cmdline.set_version(systemrdl::get_detailed_version());
    cmdline.add_option_with_optional_value(
        "a", "ast", "Enable AST JSON output, optionally specify filename");
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
        // Read input file
        std::ifstream stream(inputFile);
        if (!stream.is_open()) {
            std::cerr << "Error: Cannot open file " << inputFile << std::endl;
            return 1;
        }

        // Create ANTLR input stream
        ANTLRInputStream input(stream);

        // Create lexer
        SystemRDLLexer lexer(&input);

        // Create token stream
        CommonTokenStream tokens(&lexer);

        // Create parser
        SystemRDLParser parser(&tokens);

        // Parse, starting from root rule
        tree::ParseTree *tree = parser.root();

        // Check for syntax errors
        if (parser.getNumberOfSyntaxErrors() > 0) {
            std::cerr << "Syntax errors found: " << parser.getNumberOfSyntaxErrors() << std::endl;
            return 1;
        }

        std::cout << "[OK] Parsing successful!" << std::endl;

        // Print AST to console
        std::cout << "\n=== Abstract Syntax Tree ===" << std::endl;
        printAST(tree, &parser);

        // Generate AST JSON output if requested
        if (cmdline.is_set("ast")) {
            std::string output_file = cmdline.get_value("ast");

            // If no filename provided, generate default
            if (output_file.empty()) {
                output_file = get_default_ast_filename(inputFile, "_ast");
            }

            std::cout << "\nGenerating AST JSON output..." << std::endl;

            // Use unified API for consistent JSON output
            systemrdl::Result result = systemrdl::file::parse(inputFile);
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

        std::cout << "\n[OK] Parser completed successfully!" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
