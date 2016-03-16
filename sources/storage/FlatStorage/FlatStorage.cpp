#include "..\\interface_impl.h"
#include "..\\auxiliaryStorage.h"
#include <fstream>
#define NOMINMAX
#include <Windows.h>
#include <boost/version.hpp> 
#if defined(_MSC_VER) && BOOST_VERSION==105700 
//#pragma warning(disable:4003) 
#define BOOST_PP_VARIADICS 0 
#endif
#include <boost\scope_exit.hpp>



using namespace std;

HRESULT _CCONV FlatStorage::openStorage(const char * st)
{
	if (st == nullptr)
		return E_INVALIDARG;
	
	if (!parser.initialize(st))
		parser.setServer(st);
	if (parser.getServer().empty())
		return E_INVALIDARG;
	if (!createDir(parser.getServer()))
		return CO_E_FAILEDTOGETWINDIR;
	return S_OK;
};

FlatStorage::~FlatStorage()
{
};

//FlatBase (common for FlatStorage and SMBStorage)

HRESULT _CCONV FlatBase::add(const char * name, const char * data, const UINT size)
{
	if (name == nullptr || data == nullptr)
		return E_INVALIDARG;
	//create dir if needed
	const char * it = getFileNameFromPath(name);
	if (it != name)
	{
		const string relativePath(name, it);
		const string fullPath = makePathFile(parser.getServer(), relativePath);
		if (!createDir(fullPath))
			return CO_E_FAILEDTOGETWINDIR;
	}
	//insert file
	string newFileName = makePathFile(parser.getServer(), name);
	ofstream f(newFileName, ios::binary | ofstream::out);
	if (!f.is_open())
		return CO_E_FAILEDTOCREATEFILE;
	f.write(data, size);
	f.close();
	return SetFileAttributes(newFileName.c_str(), FILE_ATTRIBUTE_ARCHIVE) == TRUE ? S_OK : E_UNEXPECTED;
}

HRESULT _CCONV FlatBase::get(const char * name, char ** data, UINT * size)
{
	if (name == nullptr || size == nullptr)
		return E_INVALIDARG;
	string fileName = makePathFile(parser.getServer(), name);
	ifstream f(fileName, ios::binary | ifstream::in);
	if (!f.is_open())
		return S_FALSE;
	f.seekg(0, f.end);
	size_t length = static_cast<size_t>(f.tellg());
	f.seekg(0, f.beg);
	d_getFunc(data, size, length, f.read(*data, length));
	
	f.close();
	return S_OK;
}


HRESULT backupAux(const string & fromDir, const string & toDir, const FILETIME * lastBackup, UINT & amountBackup)
{
	WIN32_FIND_DATA fd;
	//HANDLE fileFind = findFirstBackup(fromDir, &fd);
	HANDLE fileFind;
	string pathToSearch = makePathFile(fromDir, "\\*");
	if ((fileFind =
		FindFirstFileEx(pathToSearch.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH))
		== INVALID_HANDLE_VALUE)
		return E_UNEXPECTED;
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
				return CO_E_FAILEDTOGETWINDIR;
			HRESULT res = backupAux(makePathFile(fromDir, fd.cFileName), copyFile, lastBackup, amountBackup);
			if (FAILED(res))
				return res;
		}
		else if (fd.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE)
		{
			if (!CopyFile(makePathFile(fromDir,fd.cFileName).c_str(), copyFile.c_str(), FALSE))
				return COMADMIN_E_CANTCOPYFILE;
			++amountBackup;
		}
	} while (FindNextFile(fileFind, &fd));
	return S_OK;
}

HRESULT _CCONV FlatBase::backupFull(const char * path, UINT * amountChanged)
{
	if (path == nullptr)
		return E_INVALIDARG;
	StringParser otherParser;
	if (!otherParser.initialize(path))
		otherParser.setServer(path);
	if (otherParser.getServer().empty())
		return E_INVALIDARG;
	if (!createDir(otherParser.getServer()))
		return CO_E_FAILEDTOGETWINDIR;
	UINT amountBackup = 0;
	HRESULT res = backupAux(parser.getServer(), otherParser.getServer(), NULL, amountBackup);
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return res;
}

//start backupIncremental
///////////////////////////////////////////////////////////////////////


//also creates file if not exists
HRESULT getLogFileData(const std::string & dir, fstream & file, string & data)
{
	string filename = makePathFile(dir, g_logName);
	file.open(filename, fstream::in | fstream::out | fstream::app | fstream::binary);
	if (!file.is_open())
		return CO_E_FAILEDTOCREATEFILE;
	data = getDataFile(file);
	file.close();
	if (SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_HIDDEN) == FALSE)
		return E_UNEXPECTED;
	return S_OK;
}

HRESULT setLogFileData(const string & dir, fstream & file, const string & data)
{
	string filename = makePathFile(dir, g_logName);
	file.open(filename, fstream::in | fstream::out | fstream::binary);
	if (!file.is_open())
		return CO_E_FAILEDTOCREATEFILE;
	file.write(data.data(), data.size());
	file.close();
	if (SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_HIDDEN) == FALSE)
		return E_UNEXPECTED;
	return S_OK;
}

HRESULT _CCONV FlatBase::backupIncremental(const char * path, UINT * amountChanged)
{
	if (path == nullptr)
		return E_INVALIDARG;

	StringParser otherParser;
	if (!otherParser.initialize(path))
		otherParser.setServer(path);
	if (otherParser.getServer().empty())
		return E_INVALIDARG;

	if (!createDir(otherParser.getServer()))
		return CO_E_FAILEDTOGETWINDIR;
	fstream file;
	string data;
	HRESULT retVal = getLogFileData(otherParser.getServer(), file, data);
	if (FAILED(retVal))
		return retVal;
	PathTimeLog log(parser.getServer(), data);
	if (log.isCorrupted())
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WINDOWS, ERROR_FILE_CORRUPT);
	UINT amountBackup = 0;
	retVal = backupAux(parser.getServer(), otherParser.getServer(), &log.getLastBackupTime(), amountBackup);
	if (FAILED(retVal))
		return retVal;
	replaceRecordInData(parser.getServer(), log.getNowTime(), data);
	retVal = setLogFileData(otherParser.getServer(), file, data);
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return retVal;
}


HRESULT _CCONV FlatBase::remove(const char * name)
{
	if (name == nullptr)
		return E_INVALIDARG;
	string fullName = makePathFile(parser.getServer(), name);
	if (DeleteFile(fullName.c_str()))
		return S_OK;
	if (GetLastError() == ERROR_FILE_NOT_FOUND)
		return S_FALSE;
	else
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WINDOWS, ERROR_ACCESS_DENIED);
}