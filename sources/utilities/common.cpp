#include "common.h"

using namespace std;

namespace utilities
{


std::string makePathFile(const std::string & path, const char * file, const char c)
{
	if (file)
	{

		if (path.empty() || path.back() == c)
			return path + file;
		else
			return path + string(1, c) + file;
	}
	else
	{
		return path;
	}
}


std::string makePathFile(const char * path, const char * file, const char c)
{
	return makePathFile(string(path), file);
}

}