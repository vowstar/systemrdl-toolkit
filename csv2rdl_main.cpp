#include "cmdline_parser.h"
#include "systemrdl_api.h"
#include "systemrdl_version.h"
#include <fstream>
#include <iostream>

int main(int argc, char *argv[])
{
    // Setup command line parser
    CmdLineParser cmdline(
        "CSV to SystemRDL Converter - Convert CSV register definitions to SystemRDL format");
    cmdline.set_version(systemrdl::get_detailed_version());
    cmdline.add_option_with_optional_value("o", "output", "Output RDL file (default: <input>.rdl)");
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
        std::cerr << "Error: No input CSV file specified" << std::endl;
        cmdline.print_help();
        return 1;
    }

    std::string input_file  = args[0];
    std::string output_file = cmdline.get_value("output");

    // Generate default output filename if not specified
    if (output_file.empty()) {
        size_t dot_pos = input_file.find_last_of('.');
        if (dot_pos != std::string::npos) {
            output_file = input_file.substr(0, dot_pos) + ".rdl";
        } else {
            output_file = input_file + ".rdl";
        }
    }

    try {
        std::cout << "[PARSE] Parsing CSV file: " << input_file << std::endl;

        // Use the API to convert CSV to RDL
        auto result = systemrdl::file::csv_to_rdl(input_file);
        if (!result.ok()) {
            std::cerr << "Error: " << result.error() << std::endl;
            return 1;
        }

        std::cout << "[OK] Successfully converted CSV to SystemRDL" << std::endl;

        // Write output file
        std::ofstream output_stream(output_file);
        if (!output_stream.is_open()) {
            std::cerr << "Error: Cannot create output file " << output_file << std::endl;
            return 1;
        }

        output_stream << result.value();
        output_stream.close();

        std::cout << "[OK] SystemRDL file generated: " << output_file << std::endl;
        std::cout << "\n[OK] Conversion completed successfully!" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
