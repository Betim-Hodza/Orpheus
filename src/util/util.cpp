// util.cpp
#include "../../include/util.hpp"
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>

namespace Util
{

/**
 * @brief expands path to home
 * @param read only string
 */
std::filesystem::path expandHome(std::string_view path)
{
  // early return if no ~
  if (path.empty() || path[0] != '~')
    return path;

  // get home env var from terminal
  const char *home = getenv("HOME");
  if (!home)
    return path;

  // tilde only (1 character size)
  if (path.size() == 1)
    return std::filesystem::path(home);

  // ~/ (2 char size) we want to just join it with the substr
  return std::filesystem::path(home) / path.substr(2);
}

bool listDir(std::string_view path, std::vector<std::string> &content_list)
{
  if (!std::filesystem::exists(path))
  {
    errorPrint(std::string("Path does not exists") + std::string(path));
    return false;
  }

  if (!std::filesystem::is_directory(path))
  {
    errorPrint(std::string("Path is not a directory") + std::string(path));
    return false;
  }

  try
  {
    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
      if (entry.is_regular_file())
      {
        content_list.push_back(entry.path().filename().string());
      }
      else
      {
        content_list.push_back(entry.path().filename().string() + "/");
      }
    }
  }
  catch (const std::filesystem::filesystem_error &ex)
  {
    errorPrint(std::string("Filesystem error") + ex.what());
    return false;
  }

  return true;
}

/**
 * @brief format total_seconds to a 00:00 clock format string
 * @param total_seconds to be formated as string
 */
std::string formatDuration(int total_seconds)
{
  int minutes = total_seconds / 60;
  int seconds = total_seconds % 60;

  return std::format("{}:{:02d}", minutes, seconds);
}

/**
 * @brief Print a debug message (only if HEIMDALL_DEBUG_ENABLED is defined)
 * @param message The message to print
 */
void debugPrint(const std::string &message)
{
#ifdef ORPHEUS_DEBUG_ENABLED
  std::cout << "[DEBUG] " << message << std::endl;
#endif
}

/**
 * @brief Print an informational message (only if HEIMDALL_DEBUG_ENABLED is
 * defined)
 * @param message The message to print
 */
void infoPrint(const std::string &message)
{
#ifdef ORPHEUS_DEBUG_ENABLED
  std::cout << "[INFO] " << message << std::endl;
#endif
}

/**
 * @brief Print an error message
 * @param message The error message to print
 */
void errorPrint(const std::string &message)
{
  std::cerr << "[ERROR] " << message << std::endl;
}

/**
 * @brief Print a warning message
 * @param message The warning message to print
 */
void warningPrint(const std::string &message)
{
  std::cerr << "[WARNING] " << message << std::endl;
}

} // namespace Util
