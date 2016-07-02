#pragma once
#include "Storage.h"
#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <map>



// for creating random files
const unsigned int g_minFileSize = 300;//300 * 1024;
const unsigned int g_maxFileSize = 5000;//5000 * 1024;


struct InitParam
{
	Storage_t type;
	std::string initParams;
};

struct ParamExtra
{
	Storage_t type;
	std::string initParams;
	std::string additionParam;
};

extern std::vector<std::pair<const std::string, std::string>> g_fileNameData;
extern std::vector<std::pair<const std::string, std::string>> g_fileNameDataIncBackup;

extern std::vector<InitParam> g_testSimpleParams;
extern std::vector<ParamExtra> g_testExportParams;
extern std::vector<ParamExtra> g_testFullBackupParams;
extern std::vector<ParamExtra> g_testIncBackupParams;


const std::map<std::string, Storage_t> g_shortcuts
{
	{ "FS", e_FSStorage },
	{ "FTP", e_FTPStorage },
	{ "SMB", e_SMBStorage },
	{ "MSSQL", e_MSSQLStorage },
	{ "PSQL", e_PostgreSQL },
	{ "SQL3", e_SQLite3 },
	{ "MDB", e_MongoDB }
};
