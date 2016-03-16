#pragma once
#include <string>
#include "Storage.h"
#include <random>

const unsigned int g_minFileSize =  300 * 1024;
const unsigned int g_maxFileSize = 5000 * 1024;

//FLAT, SMB, FTP, MsSql, PostgreSQLStorage, SQLiteStorage, MongoDB
const unsigned g_amountClasses = 7;

typedef std::ranlux48 RandomGenerator;

std::string makePathFile(const std::string & path, const std::string & file);
std::string makePathFileNorm(const std::string & path, const std::string & file);
std::string getRandData(const size_t a, const size_t b, RandomGenerator & rnd);
std::string getDataFile(std::istream & f);