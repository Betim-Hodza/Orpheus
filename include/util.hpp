// util.hpp
#pragma once
#include <filesystem>
#include <string_view>

namespace Util
{

/**
 * @brief expands path to home
 * @param read only string
 */
std::filesystem::path expandHome(std::string_view path);
/**
 * @brief format total_seconds to a 00:00 clock format string
 * @param total_seconds to be formated as string
 */
std::string formatDuration(int total_seconds); // e.g. "3:45"

/**
 * @brief Print a debug message (only if ORPHEUS_DEBUG_ENABLED is defined)
 * @param message The message to print
 */
void debugPrint(const std::string &message);

/**
 * @brief Print an informational message (only if ORPHEUS_DEBUG_ENABLED is
 * defined)
 * @param message The message to print
 */
void infoPrint(const std::string &message);

/**
 * @brief Print an error message
 * @param message The error message to print
 */
void errorPrint(const std::string &message);

/**
 * @brief Print a warning message
 * @param message The warning message to print
 */
void warningPrint(const std::string &message);
} // namespace Util
