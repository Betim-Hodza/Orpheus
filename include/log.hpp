// log.hpp
#pragma once

#include <fstream>
#include <string>

class FileLogger
{
public:
  explicit FileLogger(const std::string &filename);
  ~FileLogger();

  void log(const std::string &message);

private:
  std::ofstream log_file;
};
