#pragma once

/**
 * @file systemrdl_version.h
 * @brief SystemRDL Toolkit Version Information
 *
 * This file contains version information for the SystemRDL Toolkit.
 * It uses semantic versioning (MAJOR.MINOR.PATCH) format.
 *
 * Version components:
 * - MAJOR: Incremented for incompatible API changes
 * - MINOR: Incremented for backward-compatible functionality additions
 * - PATCH: Incremented for backward-compatible bug fixes
 */

// Version components
#define SYSTEMRDL_VERSION_MAJOR 0
#define SYSTEMRDL_VERSION_MINOR 2
#define SYSTEMRDL_VERSION_PATCH 1

// Helper macros to create version strings
#define SYSTEMRDL_STRINGIFY(x) #x
#define SYSTEMRDL_TOSTRING(x) SYSTEMRDL_STRINGIFY(x)

// Combined version string
#define SYSTEMRDL_VERSION_STRING \
    SYSTEMRDL_TOSTRING(SYSTEMRDL_VERSION_MAJOR) \
    "." SYSTEMRDL_TOSTRING(SYSTEMRDL_VERSION_MINOR) "." SYSTEMRDL_TOSTRING(SYSTEMRDL_VERSION_PATCH)

// Numeric version for programmatic comparison
#define SYSTEMRDL_VERSION_NUMBER \
    ((SYSTEMRDL_VERSION_MAJOR * 10000) + (SYSTEMRDL_VERSION_MINOR * 100) \
     + (SYSTEMRDL_VERSION_PATCH))

// Git information (to be populated by build system)
#ifndef SYSTEMRDL_GIT_COMMIT
#define SYSTEMRDL_GIT_COMMIT "unknown"
#endif

#ifndef SYSTEMRDL_GIT_BRANCH
#define SYSTEMRDL_GIT_BRANCH "unknown"
#endif

#ifndef SYSTEMRDL_BUILD_DATE
#define SYSTEMRDL_BUILD_DATE __DATE__ " " __TIME__
#endif

// Full version string with build info
#define SYSTEMRDL_FULL_VERSION_STRING \
    "SystemRDL Toolkit " SYSTEMRDL_VERSION_STRING " (built " SYSTEMRDL_BUILD_DATE ")"

// Version with git info
#define SYSTEMRDL_DETAILED_VERSION_STRING \
    "SystemRDL Toolkit " SYSTEMRDL_VERSION_STRING " [" SYSTEMRDL_GIT_BRANCH \
    "@" SYSTEMRDL_GIT_COMMIT "]" \
    " (built " SYSTEMRDL_BUILD_DATE ")"

namespace systemrdl {

/**
 * @brief Get version string
 * @return Version string in format "X.Y.Z"
 */
inline const char *get_version()
{
    return SYSTEMRDL_VERSION_STRING;
}

/**
 * @brief Get full version string with build information
 * @return Full version string with build date
 */
inline const char *get_full_version()
{
    return SYSTEMRDL_FULL_VERSION_STRING;
}

/**
 * @brief Get detailed version string with git information
 * @return Detailed version string with git branch and commit
 */
inline const char *get_detailed_version()
{
    return SYSTEMRDL_DETAILED_VERSION_STRING;
}

/**
 * @brief Get version components
 * @param major Output parameter for major version
 * @param minor Output parameter for minor version
 * @param patch Output parameter for patch version
 */
inline void get_version_components(int &major, int &minor, int &patch)
{
    major = SYSTEMRDL_VERSION_MAJOR;
    minor = SYSTEMRDL_VERSION_MINOR;
    patch = SYSTEMRDL_VERSION_PATCH;
}

/**
 * @brief Get numeric version for comparison
 * @return Numeric version (MAJOR*10000 + MINOR*100 + PATCH)
 */
inline int get_version_number()
{
    return SYSTEMRDL_VERSION_NUMBER;
}

} // namespace systemrdl
