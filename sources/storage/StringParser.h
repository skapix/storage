#pragma once
#include <string>
#include <vector>

//сервер, порт, бд, таблица, логин, пароль, ? ? ?
class StringParser
{
	std::string server,
		port,
		login,
		password,
		dbName,
		tableName;
public:
	StringParser() {}
	StringParser(const char * st) { initialize(st); }
	bool initialize(const char * st);
	StringParser(const std::string & st);
	void setServer(const char * s) { server = s; }
	const std::string & getServer() const { return server; };
	std::string & getServer() { return server; };
	const std::string & getPort() const { return port; };
	const std::string & getLogin() const { return login; };
	const std::string & getPass() const { return password; };
	const std::string & getDbName() const { return dbName; };
	const std::string & getTableName() const { return tableName; };
	
};

struct FTPLsLine
{
	enum EFileType { FILE, DIR, OTHER } typeFile;
	std::string modifiedDate;
	std::string filename;
};

extern const std::string g_month[];

std::vector<FTPLsLine> parseFTPLs(const std::string & serverResponse);