#pragma once
#include <string>
#include "Storage.h"
#include <random>
#include <map>

using RandomGenerator = std::ranlux48;

std::string toUpper(const std::string & str);
std::string getRandData(const size_t minSz, const size_t MaxSz, RandomGenerator & rnd);
std::string getDataFile(std::istream & f);
std::string uintToString(const unsigned val);