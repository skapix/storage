#define NOMINMAX
#include "auxiliary.h"
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <cassert>
#include <cstring>
#include <string>
#include <algorithm>
#include <sstream>
#include <iterator> // back_inserter
#include <cctype> // toupper

using namespace std;

typedef unsigned char byte;

string toUpper(const string & str)
{
	string retVal;
	retVal.reserve(str.size());
	transform(str.begin(), str.end(), back_inserter(retVal), std::toupper);
	return retVal;
}


string getRandData(const size_t a, const size_t b, RandomGenerator & rnd)
{
	assert(a <= b);
	const unsigned blockSize = (unsigned)(round(log(rnd.max() + 1))) / 8;
	assert(blockSize != 0);
	const size_t length = a + (rnd() % (b - a));
	string res(length, '\0');

	byte * dat = reinterpret_cast<byte *>(&res[0]);
	for (size_t i = 0; i < length + 1 - blockSize; i += blockSize)
	{
		auto rnd_val = rnd();
		memcpy(dat + i, &rnd_val, blockSize);
	}
	return res;
}


string getDataFile(istream & f)
{
	string res;
	f.seekg(0, f.end);
	size_t length = static_cast<size_t>(f.tellg());
	f.seekg(0, f.beg);
	res.resize(length);
	f.read(&res[0], length);
	return res;
}

string uintToString(const unsigned val)
{
	string ret(30, '\0');
	sprintf_s(&ret[0], 30, "%u", val);
	ret.resize(ret.find('\0'));
	return ret;
}
