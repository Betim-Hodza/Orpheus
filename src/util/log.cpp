// log.cpp
#include "../../include/log.hpp"
#include "../../include/util.hpp"
#include <fstream>
#include <iostream>

FileLogger::FileLogger(const std::string &filename) : log_file(filename, std::ios::out)
{
  if (!log_file.is_open())
  {
    Util::debugPrint("Can't open log file");
  }
}

FileLogger::~FileLogger()
{
  if (log_file.is_open())
  {
    log_file.close();
  }
}

void FileLogger::log(const std::string &message)
{
  if (log_file.is_open())
  {
    log_file << message << std::endl;
  }
  else
  {
    Util::debugPrint("Attempt to write to closed file");
  }
}

std::streambuf *FileLogger::rdbuf()
{
  return log_file.rdbuf();
}
