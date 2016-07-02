#pragma once
#include "utilities/dyn_mem_manager.h"

#include <string>
#include <iosfwd>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // FILETIME


const unsigned int g_maxFileSize = 5 * 1024 * 1024;

void bindParam(const std::string & param, const char * unbindedQuery, std::string & outputQuery);
void bind2Params(const std::string & param1, const std::string & param2,
	const char * unbindedQuery, std::string & outputQuery);
void bind3Params(const std::string & param1, const std::string & param2, const std::string & param3,
	const char * unbindedQuery, std::string & outputQuery);

const char * getFileNameFromPath(const char * path);
std::string getDataFile(std::istream & f);
bool createDir(const std::string & directory);
void replaceRecordInData(const std::string & path, const FILETIME & now, std::string & data);
FILETIME getCurrentSystemTime();
FILETIME getCurrentLocalTime();

extern const char g_logName[];
extern const char g_logDbName[];

class PathTimeLog
{
	FILETIME now;
	FILETIME lastTime;
	bool corruptedLogFile;
public:
	PathTimeLog(const std::string & st, const std::string & dat);
	FILETIME getLastBackupTime() const { return lastTime; }
	FILETIME getNowTime() const { return now; }
	bool isCorrupted() const { return corruptedLogFile; }
	~PathTimeLog() {}
private:

};

#define d_getFunc(data,size,length,func)\
 if (data != nullptr)\
 {\
	if (*size == 0)\
	{\
		*size = length;\
		*data = (char*)utilities::allocate(length);\
		if (*data == nullptr)\
			return OUTOFMEMORY;\
	}\
	else if (*size < length)\
		{\
			*size = length;\
			return INSUFFICIENTMEMORY;\
		}\
	*size = length;\
	func;\
	}\
	else\
		*size = length;

