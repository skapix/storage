#include "auxiliaryStorage.h"
#include <vector>
#include <sstream>
#include <fstream>
#include <cassert>
#include <string>
#include <algorithm>

using namespace std;

const char g_logName[] = "_syslastbackupinc.log";
const char g_logDbName[] = "_syslastbackupinc";

void bindParam(const string & param, const char * unbindedQuery, string & outputQuery)
{
	outputQuery.resize(strlen(unbindedQuery) + param.size() - 2 + 1);// +1 for nullptr//just in case
	assert(sprintf_s(&outputQuery[0], outputQuery.size(), unbindedQuery, param.c_str()) >= 0);
}

void bind2Params(const std::string & param1, const std::string & param2, const char * unbindedQuery, std::string & outputQuery)
{
	outputQuery.resize(strlen(unbindedQuery) + param1.size() + param2.size() - 4 + 1);// +1 for nullptr//just in case
	assert(sprintf_s(&outputQuery[0], outputQuery.size(), unbindedQuery, param1.c_str(), param2.c_str()) >= 0);
}

void bind3Params(const std::string & param1, const std::string & param2, const std::string & param3,
	const char * unbindedQuery, std::string & outputQuery)
{
	outputQuery.resize(strlen(unbindedQuery) + param1.size() + param2.size() + param3.size() - 6 + 1);
	assert(sprintf_s(&outputQuery[0], outputQuery.size(),
		unbindedQuery, param1.c_str(), param2.c_str(), param3.c_str()) >= 0);
}


const char * getFileNameFromPath(const char * path)
{
	const char * aux = strrchr(path, '\\');
	const char * aux2 = strrchr(path, '/');
	const char * aux3 = aux != nullptr ? (aux2 != nullptr ? std::max(aux, aux2) : aux) : aux2;
	return aux3 == nullptr ? path : aux3;
}

//check file is open and !badbit
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

bool createDir(const string & directory)
{
	bool created = CreateDirectory(directory.c_str(), NULL) == TRUE;
	if (GetLastError() == ERROR_PATH_NOT_FOUND)
	{
		size_t off = directory.find_last_of("\\/");
		if (off = string::npos)
			return false;
		string newDir(directory.begin(), directory.begin() + off);
		if (!createDir(newDir))
			return false;
		created = CreateDirectory(directory.c_str(), NULL) == TRUE;
	}
	return created || GetLastError() == ERROR_ALREADY_EXISTS;
}

//log file. data is returning value
void replaceRecordInData(const string & path, const FILETIME & now, string & data)
{
	size_t offset = 0;
	while ((offset = data.find(path, offset)) != string::npos)
	{
		if (offset && data[offset - 1] == '|')
		{
			++offset;
			continue;
		}
		size_t endOffset = data.find('\n', offset);
		endOffset = endOffset == string::npos ? data.size() : endOffset + 1;
		data.erase(data.begin() + offset, data.begin() + endOffset);
	}
	std::ostringstream os;
	os << path << '|' << now.dwHighDateTime << '|' << now.dwLowDateTime << std::endl;
	data.append(os.str());
}

FILETIME getCurrentSystemTime()
{
	FILETIME now;
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	SystemTimeToFileTime(&systime, &now);
	return now;
}

FILETIME getCurrentLocalTime()
{
	FILETIME now;
	SYSTEMTIME systime;
	GetLocalTime(&systime);
	SystemTimeToFileTime(&systime, &now);
	return now;
}

PathTimeLog::PathTimeLog(const std::string & path, const string & data)
{
	corruptedLogFile = false;
	string st;
	istringstream stream(data);
	lastTime = { 0, 0 };
	while (getline(stream, st))
	{
		if (st.empty())
			continue;
		size_t delim = st.find('|');
		if (delim == string::npos)
		{
			corruptedLogFile = true;
			return;
		}
		if (st.compare(0, delim, path) == 0)
		{
			size_t delim2 = st.find('|', delim + 1);
			if (delim2 == string::npos)
			{
				corruptedLogFile = true;
				return;
			}
			FILETIME newTime;
			string num1(st.begin() + delim + 1, st.begin() + delim2),
				num2(st.begin() + delim2 + 1, st.end());
			newTime.dwHighDateTime = stoul(num1);
			newTime.dwLowDateTime = stoul(num2);

			if (CompareFileTime(&lastTime, &newTime) == -1)
				lastTime = newTime;
		}
	}

	now = getCurrentSystemTime();
}

