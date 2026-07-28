#include "SystemRDLLexer.h"
#include "SystemRDLParser.h"
#include "cmdline_parser.h"
#include "systemrdl_api.h"
#include "systemrdl_version.h"
#include <algorithm>
#include <fstream>
#include <inja/inja.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace antlr4;

// Helper function to get file extension
std::string get_file_extension(const std::string &filename)
{
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    std::string ext = filename.substr(dot_pos + 1);
    // Convert to lowercase for case-insensitive comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

int main(int argc, char *argv[])
{
    // Setup command line parser
    CmdLineParser cmdline(
        "SystemRDL Template Renderer - Render SystemRDL designs using Jinja2 templates");
    cmdline.set_version(systemrdl::get_detailed_version());
    cmdline.add_option("t", "template", "Jinja2 template file (.j2)", true);
    cmdline
        .add_option_with_optional_value("o", "output", "Output file (default: auto-generated name)");
    cmdline.add_option(
        "", "ast", "Use full AST JSON format instead of simplified JSON (default: simplified)");
    cmdline.add_option("", "verbose", "Enable verbose output");
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
        std::cerr << "Supported formats: .rdl (SystemRDL) and .csv (CSV register definitions)"
                  << std::endl;
        cmdline.print_help();
        return 1;
    }

    if (!cmdline.is_set("template")) {
        std::cerr << "Error: Template file not specified (use -t/--template)" << std::endl;
        cmdline.print_help();
        return 1;
    }

    std::string input_file    = args[0];
    std::string template_file = cmdline.get_value("template");
    bool        verbose       = cmdline.is_set("verbose");
    bool use_ast = cmdline.is_set("ast"); // Default to simplified JSON unless --ast is specified

    // Detect input file type
    std::string file_ext = get_file_extension(input_file);
    bool        is_csv   = (file_ext == "csv");
    bool        is_rdl   = (file_ext == "rdl");

    if (!is_csv && !is_rdl) {
        std::cerr << "Error: Unsupported file format '" << file_ext << "'" << std::endl;
        std::cerr << "Supported formats: .rdl (SystemRDL) and .csv (CSV register definitions)"
                  << std::endl;
        return 1;
    }

    if (verbose) {
        std::cout << "Processing " << (is_csv ? "CSV" : "RDL") << " file: " << input_file
                  << std::endl;
        std::cout << "Using template: " << template_file << std::endl;
        std::cout << "Output format: " << (use_ast ? "Full AST JSON" : "Simplified JSON (default)")
                  << std::endl;
    }

    try {
        // Get elaborated JSON - different path for CSV vs RDL
        auto elaborate_result = systemrdl::Result::success("");

        if (is_csv) {
            // CSV -> RDL -> Elaborate
            if (verbose) {
                std::cout << "Converting CSV to SystemRDL..." << std::endl;
            }

            auto csv_to_rdl_result = systemrdl::file::csv_to_rdl(input_file);
            if (!csv_to_rdl_result.ok()) {
                std::cerr << "CSV to RDL conversion failed: " << csv_to_rdl_result.error()
                          << std::endl;
                return 1;
            }

            if (verbose) {
                std::cout << "Successfully converted CSV to SystemRDL" << std::endl;
                std::cout << "SystemRDL preview:" << std::endl;
                std::cout << csv_to_rdl_result.value().substr(0, 300) << "..." << std::endl;
            }

            // Now elaborate the generated RDL content
            if (use_ast) {
                elaborate_result = systemrdl::elaborate(csv_to_rdl_result.value());
            } else {
                elaborate_result = systemrdl::elaborate_simplified(csv_to_rdl_result.value());
            }
        } else {
            // Direct RDL -> Elaborate
            if (use_ast) {
                elaborate_result = systemrdl::file::elaborate(input_file);
            } else {
                elaborate_result = systemrdl::file::elaborate_simplified(input_file);
            }
        }

        if (!elaborate_result.ok()) {
            std::cerr << "Elaboration failed: " << elaborate_result.error() << std::endl;
            return 1;
        }

        if (verbose) {
            std::cout << "Successfully elaborated SystemRDL design" << std::endl;
            std::cout << "Using " << (use_ast ? "full AST" : "simplified") << " JSON format"
                      << std::endl;
        }

        // Parse the JSON string to nlohmann::json for Inja
        json elaborated_json;
        try {
            elaborated_json = json::parse(elaborate_result.value());
        } catch (const json::parse_error &e) {
            std::cerr << "Failed to parse elaborated JSON: " << e.what() << std::endl;
            return 1;
        }

        if (verbose) {
            std::cout << "JSON structure preview:" << std::endl;
            std::cout << elaborated_json.dump(2).substr(0, 500) << "..." << std::endl;
        }

        // Setup Inja environment
        inja::Environment env;
        // Disable line statements
        env.set_line_statement("");
        // Render template
        std::string rendered_content = env.render_file(template_file, elaborated_json);

        if (verbose) {
            std::cout << "Successfully rendered template" << std::endl;
        }

        // Generate output filename if not specified
        std::string output_file = cmdline.get_value("output");
        if (output_file.empty()) {
            std::string input_basename = input_file.substr(input_file.find_last_of("/\\") + 1);
            size_t      dot_pos        = input_basename.find_last_of('.');
            if (dot_pos != std::string::npos) {
                input_basename.resize(dot_pos);
            }

            std::string template_basename = template_file.substr(
                template_file.find_last_of("/\\") + 1);

            // Extract purpose and extension from template name (e.g., test_j2_header.h.j2 -> header.h)
            size_t j2_pos = template_basename.find("_j2_");
            if (j2_pos != std::string::npos) {
                size_t start = j2_pos + 4;
                size_t end   = template_basename.find(".j2");
                if (end != std::string::npos) {
                    std::string purpose_and_ext = template_basename.substr(start, end - start);
                    output_file                 = input_basename + "_" + purpose_and_ext;
                }
            }

            if (output_file.empty()) {
                output_file = input_basename + "_rendered.txt";
            }
        }

        // Write output
        std::ofstream out_file(output_file);
        if (!out_file) {
            std::cerr << "Error: Cannot write to output file: " << output_file << std::endl;
            return 1;
        }

        out_file << rendered_content;
        out_file.close();

        if (verbose) {
            std::cout << "Output written to: " << output_file << std::endl;
        } else {
            std::cout << output_file << std::endl;
        }

        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
