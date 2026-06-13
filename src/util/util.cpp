// util.cpp
#include "util.hpp"
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>
#include <numeric>
#include <algorithm>

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

bool listDir(std::string_view path, std::vector<std::string> &content_list, std::vector<ItemType> &content_items)
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
    // clear all lists
    content_list.clear();
    content_items.clear();
    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
      if (entry.is_regular_file())
      {
        content_items.push_back(ItemType::Song);
        content_list.push_back(entry.path().filename().string());
      }
      else
      {
        content_items.push_back(ItemType::Directory);
        content_list.push_back(entry.path().filename().string() + "/");
      }
    }

		// sort alphabetically (use iota to enumerate)
		std::vector<size_t> indices(content_list.size());
		std::iota(indices.begin(), indices.end(), 0);

		// O(N log(n) L) L = string comparison, std::sort is N Log(N)
		std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
			return content_list[a] < content_list[b];
		});
		
		std::vector<std::string> sorted_list(content_list.size());
		std::vector<ItemType> sorted_item(content_items.size());

		// place known list of dir in a sorted fashion using indices iota
		for (size_t i = 0; i < indices.size(); ++i)
		{
			sorted_list[i] = content_list[indices[i]];
			sorted_item[i] = content_items[indices[i]];
		}

		// reassign both vectors with the sorted version
		content_list = sorted_list;
		content_items = sorted_item;

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
  std::cout << "[DEBUG] " << message << std::endl;
}

/**
 * @brief Print an informational message (only if HEIMDALL_DEBUG_ENABLED is
 * defined)
 * @param message The message to print
 */
void infoPrint(const std::string &message)
{
  std::cout << "[INFO] " << message << std::endl;
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
