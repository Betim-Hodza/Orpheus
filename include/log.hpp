// log.hpp
#pragma once

#include <fstream>
#include <iostream>
#include <string>

class FileLogger
{
public:
  explicit FileLogger(const std::string &filename);
  ~FileLogger();

  void log(const std::string &message);
  std::streambuf *rdbuf();

private:
  std::ofstream log_file;
};
