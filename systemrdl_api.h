#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace systemrdl {

/**
 * @brief Result type for SystemRDL API operations
 *
 * A simple result type that can hold either a successful value or an error message.
 * Similar to std::expected but simpler and more focused.
 */
struct Result
{
    std::string value_;
    std::string error_;
    bool        is_success;

    // Private constructor - use static factory methods
    Result(std::string val, std::string err, bool success)
        : value_(std::move(val))
        , error_(std::move(err))
        , is_success(success)
    {}

public:
    // Factory methods for creating results
    static Result success(std::string value) { return Result(std::move(value), "", true); }

    static Result error(std::string error_msg) { return Result("", std::move(error_msg), false); }

    // Query methods
    bool ok() const { return is_success; }
    bool has_error() const { return !is_success; }

    // Value access (only call if ok() returns true)
    const std::string &value() const { return value_; }
    std::string       &value() { return value_; }

    // Error access (only call if has_error() returns true)
    const std::string &error() const { return error_; }
};

/**
 * @brief Parse SystemRDL content and generate JSON AST
 *
 * @param rdl_content The SystemRDL content to parse
 * @return Result containing JSON AST string on success, or error message on failure
 *
 * @example
 * ```cpp
 * std::string rdl_content = "addrmap simple { reg r1 @ 0x0; };";
 * auto result = systemrdl::parse(rdl_content);
 * if (result.ok()) {
 *     std::cout << "AST JSON: " << result.value() << std::endl;
 * } else {
 *     std::cerr << "Parse error: " << result.error() << std::endl;
 * }
 * ```
 */
Result parse(std::string_view rdl_content);

/**
 * @brief Parse and elaborate SystemRDL content, generate JSON elaborated model
 *
 * @param rdl_content The SystemRDL content to elaborate
 * @return Result containing JSON elaborated model on success, or error message on failure
 *
 * @example
 * ```cpp
 * std::string rdl_content = "addrmap simple { reg r1 @ 0x0; };";
 * auto result = systemrdl::elaborate(rdl_content);
 * if (result.ok()) {
 *     std::cout << "Elaborated JSON: " << result.value() << std::endl;
 * } else {
 *     std::cerr << "Elaboration error: " << result.error() << std::endl;
 * }
 * ```
 */
Result elaborate(std::string_view rdl_content);

/**
 * @brief Convert CSV content to SystemRDL format
 *
 * @param csv_content The CSV content to convert
 * @return Result containing SystemRDL string on success, or error message on failure
 *
 * @example
 * ```cpp
 * std::string csv_content = "addrmap_name,reg_name,field_name,field_lsb,field_msb\n"
 *                           "DEMO,CTRL,ENABLE,0,0\n";
 * auto result = systemrdl::csv_to_rdl(csv_content);
 * if (result.ok()) {
 *     std::cout << "SystemRDL: " << result.value() << std::endl;
 * } else {
 *     std::cerr << "Conversion error: " << result.error() << std::endl;
 * }
 * ```
 */
Result csv_to_rdl(std::string_view csv_content);

/**
 * @brief Utility functions for file-based operations
 */
namespace file {

/**
 * @brief Parse SystemRDL file and generate JSON AST
 *
 * @param filename Path to the SystemRDL file
 * @return Result containing JSON AST string on success, or error message on failure
 */
Result parse(const std::string &filename);

/**
 * @brief Parse and elaborate SystemRDL file, generate JSON elaborated model
 *
 * @param filename Path to the SystemRDL file
 * @return Result containing JSON elaborated model on success, or error message on failure
 */
Result elaborate(const std::string &filename);

/**
 * @brief Convert CSV file to SystemRDL format
 *
 * @param filename Path to the CSV file
 * @return Result containing SystemRDL string on success, or error message on failure
 */
Result csv_to_rdl(const std::string &filename);

} // namespace file

/**
 * @brief Streaming interface for large content processing
 */
namespace stream {

/**
 * @brief Parse SystemRDL from input stream and write JSON AST to output stream
 *
 * @param input Input stream containing SystemRDL content
 * @param output Output stream to write JSON AST
 * @return true on success, false on failure
 */
bool parse(std::istream &input, std::ostream &output);

/**
 * @brief Parse and elaborate SystemRDL from input stream, write JSON to output stream
 *
 * @param input Input stream containing SystemRDL content
 * @param output Output stream to write JSON elaborated model
 * @return true on success, false on failure
 */
bool elaborate(std::istream &input, std::ostream &output);

/**
 * @brief Convert CSV from input stream to SystemRDL in output stream
 *
 * @param input Input stream containing CSV content
 * @param output Output stream to write SystemRDL
 * @return true on success, false on failure
 */
bool csv_to_rdl(std::istream &input, std::ostream &output);

} // namespace stream

} // namespace systemrdl
