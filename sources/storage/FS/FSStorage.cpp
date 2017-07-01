#include "FSStorage.h"
#include "../auxiliaryStorage.h"
#include "utilities/common.h"
#include <fstream>
#define NOMINMAX
#include <Windows.h>

#include <boost/scope_exit.hpp>



using namespace std;
using utilities::makePathFile;

ErrorCode _CCONV FSStorage::openStorage(const char * st)
{
	if (st == nullptr)
		return INVALIDARG;
	
	if (!parser.initialize(st))
		parser.setServer(st);

	if (parser.getServer().empty())
		return INVALIDARG;

	if (!createDir(parser.getServer()))
		return FAILEDTOGETWINDIR;

	return OK;
};

FSStorage::~FSStorage()
{};

//FSBase (common for FSStorage and SMBStorage)

ErrorCode _CCONV FSBase::add(const char * name, const char * data, const unsigned size)
{
	if (name == nullptr || data == nullptr)
		return INVALIDARG;
	//create dir if needed
	const char * it = getFileNameFromPath(name);
	if (it != name)
	{
		const string relativePath(name, it);
		const string fullPath = makePathFile(parser.getServer(), relativePath);
		if (!createDir(fullPath))
			return FAILEDTOGETWINDIR;
	}
	//insert file
	string newFileName = makePathFile(parser.getServer(), name);
	ofstream f(newFileName, ios::binary | ofstream::out);
	if (!f.is_open())
		return FAILEDTOCREATEFILE;
	f.write(data, size);
	f.close();
	return SetFileAttributes(newFileName.c_str(), FILE_ATTRIBUTE_ARCHIVE) == TRUE ? OK : UNEXPECTED;
}

ErrorCode _CCONV FSBase::get(const char * name, char ** data, unsigned * size)
{
	if (name == nullptr || size == nullptr)
		return INVALIDARG;
	string fileName = makePathFile(parser.getServer(), name);
	ifstream f(fileName, ios::binary | ifstream::in);
	if (!f.is_open())
		return EC_FALSE;
	f.seekg(0, f.end);
	size_t length = static_cast<size_t>(f.tellg());
	f.seekg(0, f.beg);
	d_getFunc(data, size, length, f.read(*data, length));
	
	f.close();
	return OK;
}


ErrorCode backupAux(const string & fromDir, const string & toDir, const FILETIME * lastBackup, unsigned & amountBackup)
{
	WIN32_FIND_DATA fd;
	//HANDLE fileFind = findFirstBackup(fromDir, &fd);
	HANDLE fileFind;
	string pathToSearch = makePathFile(fromDir, "\\*");
	if ((fileFind =
		FindFirstFileEx(pathToSearch.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH))
		== INVALID_HANDLE_VALUE)
		return UNEXPECTED;
	BOOST_SCOPE_EXIT(fileFind){ FindClose(fileFind); } BOOST_SCOPE_EXIT_END
	//skip "." and ".."
	while (fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY &&
		(strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) && FindNextFile(fileFind, &fd));

	do
	{
		if (lastBackup && CompareFileTime(&fd.ftLastWriteTime, lastBackup) == -1)
			continue;
		string copyFile = makePathFile(toDir, fd.cFileName);
		if (fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			
			if (!createDir(copyFile))
				return FAILEDTOGETWINDIR;
			ErrorCode res = backupAux(makePathFile(fromDir, fd.cFileName), copyFile, lastBackup, amountBackup);
			if (failed(res))
				return res;
		}
		else if (fd.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE)
		{
			if (!CopyFile(makePathFile(fromDir,fd.cFileName).c_str(), copyFile.c_str(), FALSE))
				return CANTCOPYFILE;
			++amountBackup;
		}
	} while (FindNextFile(fileFind, &fd));
	return OK;
}

ErrorCode _CCONV FSBase::backupFull(const char * path, unsigned * amountChanged)
{
	if (path == nullptr)
		return INVALIDARG;
	StringParser otherParser;
	if (!otherParser.initialize(path))
		otherParser.setServer(path);
	if (otherParser.getServer().empty())
		return INVALIDARG;
	if (!createDir(otherParser.getServer()))
		return FAILEDTOGETWINDIR;
	unsigned amountBackup = 0;
	ErrorCode res = backupAux(parser.getServer(), otherParser.getServer(), NULL, amountBackup);
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return res;
}

//start backupIncremental
///////////////////////////////////////////////////////////////////////


//also creates file if not exists
ErrorCode getLogFileData(const std::string & dir, fstream & file, string & data)
{
	string filename = makePathFile(dir, g_logName);
	file.open(filename, fstream::in | fstream::out | fstream::app | fstream::binary);
	if (!file.is_open())
		return FAILEDTOCREATEFILE;
	data = getDataFile(file);
	file.close();
	if (SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_HIDDEN) == FALSE)
		return UNEXPECTED;
	return OK;
}

ErrorCode setLogFileData(const string & dir, fstream & file, const string & data)
{
	string filename = makePathFile(dir, g_logName);
	file.open(filename, fstream::in | fstream::out | fstream::binary);
	if (!file.is_open())
		return FAILEDTOCREATEFILE;
	file.write(data.data(), data.size());
	file.close();
	if (SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_HIDDEN) == FALSE)
		return UNEXPECTED;
	return OK;
}

ErrorCode _CCONV FSBase::backupIncremental(const char * path, unsigned * amountChanged)
{
	if (path == nullptr)
		return INVALIDARG;

	StringParser otherParser;
	if (!otherParser.initialize(path))
		otherParser.setServer(path);
	if (otherParser.getServer().empty())
		return INVALIDARG;

	if (!createDir(otherParser.getServer()))
		return FAILEDTOGETWINDIR;
	fstream file;
	string data;
	ErrorCode retVal = getLogFileData(otherParser.getServer(), file, data);
	if (failed(retVal))
		return retVal;
	PathTimeLog log(parser.getServer(), data);
	if (log.isCorrupted())
		return LOG_CORRUPTED;
	unsigned amountBackup = 0;
	retVal = backupAux(parser.getServer(), otherParser.getServer(), &log.getLastBackupTime(), amountBackup);
	if (failed(retVal))
		return retVal;
	replaceRecordInData(parser.getServer(), log.getNowTime(), data);
	retVal = setLogFileData(otherParser.getServer(), file, data);
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return retVal;
}


ErrorCode _CCONV FSBase::remove(const char * name)
{
	if (name == nullptr)
		return INVALIDARG;
	string fullName = makePathFile(parser.getServer(), name);
	if (DeleteFile(fullName.c_str()))
		return OK;
	if (GetLastError() == ERROR_FILE_NOT_FOUND)
		return EC_FALSE;
	else
		return ACCESS_DENIES;
}