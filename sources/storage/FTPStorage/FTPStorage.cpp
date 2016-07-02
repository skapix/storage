#define NOMINMAX
#include "../interface_impl.h"
#include "curl/curl.h"
#include "../auxiliaryStorage.h"
#include "utilities/common.h"
#include <boost/scope_exit.hpp>



//using namespace std;
using std::string;
using std::vector;
using utilities::makePathFile;

//assert(sizeof(char))==1
struct DataGetStruct
{
	char * buf;
	UINT size;
};

struct DataSendStruct
{
	const char * buf;
	UINT size;
};

static size_t getSizeFromHeaderCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	//(void)ptr;
	//(void)data;
	char * p = (char*)ptr;
	/* we are not interested in the headers itself,
	so we only return the size we would have saved ... */
	return (size_t)(size * nmemb);
}

//read from file (get)
static size_t fwriteGetStruct(void *buffer, size_t size, size_t nmemb, void *stream)
{
	DataGetStruct & dat = *(DataGetStruct*)stream;
	size_t bytes = size*nmemb;

	if (dat.size < bytes)
		return -1;//error
	memcpy(dat.buf, buffer, bytes);
	dat.buf += bytes;
	dat.size -= bytes;
	return bytes;
}


static size_t freadSendStruct(void *ptr, size_t size, size_t nmemb, void *stream)
{
	DataSendStruct *s = (DataSendStruct *)stream;
	size_t readSz = std::min(size*nmemb, s->size);
	memcpy(ptr, s->buf, readSz);
	s->size -= readSz;
	s->buf += readSz;
	return readSz;
}


//port, login, pwd
ErrorCode setCurlParams(const StringParser & parser, CURL * curl)
{
	bool retVal = true;
	if (!parser.getPort().empty())
	{
		unsigned long port = std::stoul(parser.getPort());
		if (port >=std::numeric_limits<unsigned short>::max())
			return INVALIDARG;
		if (curl_easy_setopt(curl, CURLOPT_PORT, (unsigned short)port) != CURLE_OK)
			return SYSTEMAPP;
	}
	if (!parser.getLogin().empty())
	{
		if (curl_easy_setopt(curl, CURLOPT_USERNAME, parser.getLogin().c_str()) != CURLE_OK)
			return SYSTEMAPP;
	}
	if (!parser.getPass().empty())
	{
		if (curl_easy_setopt(curl, CURLOPT_PASSWORD, parser.getPass().c_str()) != CURLE_OK)
			return SYSTEMAPP;
	}
	return OK;
}

ErrorCode _CCONV FTPStorage::openStorage(const char * dataPath)
{
	if (dataPath == nullptr || !parser.initialize(dataPath))
		return INVALIDARG;
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	exportCurl = curl_easy_init();
	if (!curl || !exportCurl)
		return SYSTEMAPP;

	if (curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L) != CURLE_OK || 
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwriteGetStruct) != CURLE_OK || //for get, string as par
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, freadSendStruct) != CURLE_OK || //for add, string as par
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, getSizeFromHeaderCallback) != CURLE_OK || //header func
		curl_easy_setopt(curl, CURLOPT_HEADER, 0L) != CURLE_OK || //No header output
		curl_easy_setopt(exportCurl, CURLOPT_WRITEFUNCTION, fwrite) != CURLE_OK) //for export, file* as par
		return SYSTEMAPP;

	ErrorCode res = setCurlParams(parser, curl);
	if (failed(res))
		return res;
	return setCurlParams(parser, exportCurl);
}


//assert *size >= sizeof(downloading file)
ErrorCode receiveFile(CURL * curl, const char * url, char * data, const UINT size)
{
	CURLcode code = CURLE_OK;
	DataGetStruct s = { data, size };
	do
	{
		if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s) != CURLE_OK)
			return SYSTEMAPP;
		code = curl_easy_perform(curl);
	}
	while (code == CURLE_PARTIAL_FILE);
	return code == CURLE_OK ? OK : EC_FALSE;
}


ErrorCode getFileSize(CURL * curl, const char * fileurl, UINT & filesize)
{
	curl_easy_setopt(curl, CURLOPT_URL, fileurl);
	if (curl_easy_setopt(curl, CURLOPT_NOBODY, 1L) != CURLE_OK)
		return SYSTEMAPP;
	BOOST_SCOPE_EXIT(curl){ curl_easy_setopt(curl, CURLOPT_NOBODY, 0L); } BOOST_SCOPE_EXIT_END;
	//curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
	//CURLINFO_HEADER_SIZE
	const UINT bufSz = 19 + 22 + 10;
	char b[bufSz];
	ErrorCode retVal = receiveFile(curl, fileurl, b, bufSz);
	if (failed(retVal) || retVal == S_FALSE)
		return retVal;
	
	
	double d_fileSize = -1.;
	CURLcode res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &d_fileSize);
	
	if (res != CURLE_OK)
		return FAIL;
	else if (d_fileSize < 0.0)
		return EC_FALSE;
	if (d_fileSize > std::numeric_limits<unsigned int>::max())
		return FILE_TOO_LARGE;
	filesize = static_cast<UINT>(ceil(d_fileSize));
	return OK;
}

ErrorCode sendFile(CURL * curl, const string & path, const char * filename, const char * data, const UINT size)
{
	DataSendStruct s = { data, size };
	string fileLoc = makePathFile(path, filename, '/');
	if (curl_easy_setopt(curl, CURLOPT_URL, fileLoc.c_str()) != CURLE_OK ||
		curl_easy_setopt(curl, CURLOPT_READDATA, &s) != CURLE_OK)
		return SYSTEMAPP;
	if (curl_easy_perform(curl) != CURLE_OK)
		return FILE_WRITEFAIL;
	return OK;
}


ErrorCode _CCONV FTPStorage::add(const char * name, const char * data, const UINT size)
{
	if (name == nullptr || data == nullptr)
		return INVALIDARG;
	if (curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L) != CURLE_OK)
		return SYSTEMAPP;
	ErrorCode res = sendFile(curl, parser.getServer(), name, data, size);
	if (curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L) != CURLE_OK)
		return SYSTEMAPP;
	return res;
}


ErrorCode _CCONV FTPStorage::get(const char * name, char ** data, UINT * size)
{
	if (size == nullptr)
		return INVALIDARG;
	string pathFile = makePathFile(parser.getServer(), name, '/');
	UINT filesize = 0;
	ErrorCode retVal = getFileSize(curl, pathFile.c_str(), filesize);
	if (failed(retVal))
		return retVal;
	UINT length = static_cast<UINT>(filesize);

	d_getFunc(data, size, length, return receiveFile(curl, pathFile.c_str(), *data, *size));
	return OK;
}



ErrorCode _CCONV FTPStorage::exportFiles(const char * const * fileNames, const UINT amount, const char * path)
{
	if (fileNames == nullptr)
		return INVALIDARG;
	for (UINT i = 0; i < amount; i++)
	{
		FILE * file = NULL;
		const char * name = getFileNameFromPath(fileNames[i]);
		string locFile = makePathFile(path, name);
		
		
		string url = makePathFile(parser.getServer(), fileNames[i], '/');
		CURLcode cd = CURLE_OK;
		do
		{
			if (fopen_s(&file, locFile.c_str(), "wb") || file == NULL)
				return FAILEDTOCREATEFILE;
			if (curl_easy_setopt(exportCurl, CURLOPT_URL, url.c_str()) != CURLE_OK ||
				curl_easy_setopt(exportCurl, CURLOPT_WRITEDATA, file) != CURLE_OK)
				return SYSTEMAPP;
			cd = curl_easy_perform(exportCurl);
			fclose(file);
		} while (cd == CURLE_PARTIAL_FILE);
		
		if (cd != CURLE_OK)
			return FILE_READFAIL;
		
		//if fails, usualy has CURLE_PARTIAL_FILE error. Don't know how to struggle with it
		//That typically means a bad server or a network problem, not a client - side problem. <-- From stackoverflow
		//If you cannot affect the network or server conditions, you probably need to consider ignoring this particular error.
		
	}
	return OK;
}

//////////////////////////////////////////////////////////////////
//backup

bool retrieveFileTime(const string & stime, FILETIME & filetime)
{
	assert(stime.size() == 12);
	SYSTEMTIME t;
	ZeroMemory(&t, sizeof(SYSTEMTIME));
	string s(stime.begin(), stime.begin() + 3);
	t.wMonth = std::find(g_month, g_month + 12, s) - g_month + 1;
	assert(t.wMonth != 13);
	s.assign(stime.begin() + 4, stime.begin() + 6);
	t.wDay = std::stoi(s);
	s.assign(stime.begin() + 7, stime.end());
	
	FILETIME addingValue = { 0, 0 };
	if (s[2] == ':')
	{
		t.wHour = std::stoi(s);
		s.assign(stime.begin() + 10, stime.end());
		t.wMinute = std::stoi(s);
		
		SYSTEMTIME now;
		GetSystemTime(&now);
		t.wYear = now.wYear;
		//adding 1 minute to the result.
		//1 in filetime::dwLowDateTime ~ 100 ns
		addingValue.dwLowDateTime = 60 * 10000000; //1 minute
	}
	else
	{
		t.wYear = std::stoi(s);
		//adding 1 day to the result; 60 * 10000000 * 60 * 24; in hex = 0xC9 2A69C000
		addingValue.dwLowDateTime = 0x2A69C000;
		addingValue.dwHighDateTime = 0xC9;
	}

	bool retVal = SystemTimeToFileTime(&t, &filetime) == TRUE;
	DWORD newLow = filetime.dwLowDateTime + addingValue.dwLowDateTime;//add minute/part of day. FTP is so special...
	if (newLow < filetime.dwLowDateTime)
		++filetime.dwHighDateTime;
	filetime.dwLowDateTime = newLow;
	filetime.dwHighDateTime += addingValue.dwHighDateTime;
	return retVal;
}


static size_t fwriteString(void *buffer, size_t size, size_t nmemb, void *stream)
{
	string & dat = *(string*)stream, aux;
	aux.resize(size*nmemb);
	memcpy(&aux[0], buffer, size*nmemb);
	dat.append(aux);
	return size*nmemb;
}

//if fullBackup -> lastBackup == {0,0}
ErrorCode ftpBackup(CURL * curl, const string & path, CURL * backupCurl, const string & backupPath, 
	const FILETIME & lastBackup, UINT & amountBackup)
{
	string ls;
	CURLcode res = CURLE_OK;
	do
	{
		curl_easy_setopt(curl, CURLOPT_URL, path.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ls);
		CURLcode res = curl_easy_perform(curl);
	} while (res == CURLE_PARTIAL_FILE);
	if (res != CURLE_OK)
		return FILE_READFAIL;
	bool incBackup = lastBackup.dwHighDateTime != 0 || lastBackup.dwLowDateTime != 0;
	FILETIME modifiedTime;
	/* Check for errors */
	const vector<FTPLsLine> lsParsed = parseFTPLs(ls);
	for (size_t i = 0; i < lsParsed.size(); i++)
	{
		if (incBackup)
		{
			if (!retrieveFileTime(lsParsed[i].modifiedDate, modifiedTime))
				return LOG_CORRUPTED;
			if (CompareFileTime(&modifiedTime, &lastBackup) == -1)
				continue;
		}
		if (lsParsed[i].typeFile == FTPLsLine::FILE)
		{
l_againLoop:
			string data;
			string url = makePathFile(path, lsParsed[i].filename, '/');
			//get file
			if (curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) != CURLE_OK ||
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data) != CURLE_OK)
				return SYSTEMAPP;
			res = curl_easy_perform(curl);
			if (res != CURLE_OK)
			{
				if (res == CURLE_PARTIAL_FILE)
					goto l_againLoop;
				else
					return FILE_READFAIL;
			}
				
			DataSendStruct dataSend = { data.data(), (UINT)data.size() };
			if (curl_easy_setopt(backupCurl, CURLOPT_URL, 
				makePathFile(backupPath, lsParsed[i].filename).c_str(), '/') != CURLE_OK ||
				curl_easy_setopt(backupCurl, CURLOPT_READDATA, &dataSend) != CURLE_OK)
				return SYSTEMAPP;
			res = curl_easy_perform(backupCurl);
			if (res != CURLE_OK)
				return FILE_WRITEFAIL;
			++amountBackup;
		}
		else if (lsParsed[i].typeFile == FTPLsLine::DIR)
		{
			string newPath = makePathFile(path, lsParsed[i].filename, '/'),
				newBackupPath = makePathFile(backupPath, lsParsed[i].filename, '/');
			newPath.push_back('/');
			newBackupPath.push_back('/');
			ErrorCode retVal = OK;
			if (failed(retVal = ftpBackup(curl, newPath, backupCurl, newBackupPath, modifiedTime, amountBackup)))
				return retVal;
		}//if typeFile
	}//for
	return OK;
}


void prepareCurlForBackup(CURL * backupCurl, const StringParser & otherParser)
{
	setCurlParams(otherParser, backupCurl);
	curl_easy_setopt(backupCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);
	curl_easy_setopt(backupCurl, CURLOPT_READFUNCTION, freadSendStruct);
	curl_easy_setopt(backupCurl, CURLOPT_UPLOAD, 1L);
}

ErrorCode _CCONV FTPStorage::backupAux(const StringParser & otherParser, const bool incremental, UINT & amountBackup)
{
	CURL * backupCurl = curl_easy_init();
	if (!backupCurl)
		return SYSTEMAPP;
	BOOST_SCOPE_EXIT(backupCurl){ curl_easy_cleanup(backupCurl); } BOOST_SCOPE_EXIT_END;
	if (parser.getServer().back() != '/')
		parser.getServer().push_back('/');
	prepareCurlForBackup(backupCurl, otherParser);
	FILETIME lastBackup = { 0, 0 };
	//retrieve log file from otherParser.getServer
	if (incremental)
	{
		string data;
		string logurl = makePathFile(otherParser.getServer(), g_logName, '/');
		FILETIME now = getCurrentLocalTime();
		if (curl_easy_setopt(curl, CURLOPT_URL, logurl.c_str()) != CURLE_OK ||
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data) != CURLE_OK)
			return SYSTEMAPP;
		if (curl_easy_perform(curl) == CURLE_OK)
		{
			PathTimeLog log(parser.getServer(), data);
			lastBackup = log.getLastBackupTime();
		}
		ErrorCode retVal = ftpBackup(curl, parser.getServer(), backupCurl, otherParser.getServer(), lastBackup, amountBackup);
		if (succeeded(retVal))
		{
			replaceRecordInData(parser.getServer(), now, data);
			retVal = sendFile(backupCurl, otherParser.getServer(), g_logName, data.data(), data.size());
		}
		return retVal;
	}//incremental
	else
		return ftpBackup(curl, parser.getServer(), backupCurl, otherParser.getServer(), lastBackup, amountBackup);
	
}


//parser.getServer().c_str();// should have slash at the end!
ErrorCode _CCONV FTPStorage::backupFull(const char * path, UINT * amountChanged)
{
	if (path == nullptr)
		return INVALIDARG;
	if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwriteString) != CURLE_OK)
		return SYSTEMAPP;
	StringParser otherParser(path);
	if (otherParser.getServer().empty())
		return INVALIDARG;
	UINT amountBackup = 0;
	ErrorCode retVal = backupAux(path, false, amountBackup);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwriteGetStruct);
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return retVal;

}

ErrorCode _CCONV FTPStorage::backupIncremental(const char * path, UINT * amountChanged)
{
	if (path == nullptr)
		return INVALIDARG;
	if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwriteString) != CURLE_OK)
		return SYSTEMAPP;
	StringParser otherParser(path);
	if (otherParser.getServer().empty())
		return INVALIDARG;
	UINT amountBackup = 0;
	ErrorCode retVal = backupAux(path, true, amountBackup);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwriteGetStruct);
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return retVal;
}

//doesn't work  properly :(
ErrorCode _CCONV FTPStorage::remove(const char * name)
{
	if (name == nullptr)
		return INVALIDARG;
	string url = makePathFile(parser.getServer(), name, '/');
	string dele = string("DELE ") + name;
	//curl does not support removing files from ftp server by default
	//a bit tricky
	curl_slist * pHeaderList = NULL;
	pHeaderList = curl_slist_append(pHeaderList, dele.c_str());
	if (curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) != CURLE_OK ||
		curl_easy_setopt(curl, CURLOPT_QUOTE, pHeaderList) != CURLE_OK)
		return SYSTEMAPP;
	CURLcode c = curl_easy_perform(curl);
	curl_slist_free_all(pHeaderList);
	if (c == CURLE_OK)
		return OK;
	else if (c == CURLE_REMOTE_FILE_NOT_FOUND)
		return EC_FALSE;
	else
		return UNEXPECTED;
	
}

FTPStorage::~FTPStorage()
{
	if (curl)
		curl_easy_cleanup(curl);
	if (exportCurl)
		curl_easy_cleanup(exportCurl);
	curl_global_cleanup();
}