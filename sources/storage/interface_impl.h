#pragma once
#include "Storage.h"
#include "StringParser.h"


/** \brief Implements most common methods/members of all storages

  Implements exportFiles, which is common for storages
*/
struct Storage_impl : public Storage
{
protected:
	StringParser parser;
public:
	Storage_impl() {}
	virtual ErrorCode _CCONV exportFiles(const char * const * fileNames, const unsigned amount, const char * path) override;
	virtual ~Storage_impl() {}
};

/** \brief Base class for FS and SMB storages
*/
class FSBase : public Storage_impl
{
public:
	FSBase() {}
	ErrorCode _CCONV add(const char * name, const char * data, const unsigned size) override;
	ErrorCode _CCONV get(const char * name, char ** data, unsigned * size) override;
	//ErrorCode _CCONV exportFiles(const char * const * fileNames, const unsigned amount, const char * path); // implemented in Storage_impl
	ErrorCode _CCONV backupFull(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV backupIncremental(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV remove(const char * name);
	~FSBase() {};
};

class FSStorage : public FSBase
{
public:
	FSStorage() {}
	ErrorCode _CCONV openStorage(const char * name) override;
	~FSStorage() override;
};

class SMBStorage : public FSBase
{
	void * nr; //LPNETRESOURCE
public:
	SMBStorage() : nr(nullptr) {}
	ErrorCode _CCONV openStorage(const char * name) override;
	~SMBStorage() override;
};


//typedef void CURL;
class StringParser;
class FTPStorage : public Storage_impl
{
	void * curl, *exportCurl; //CURL
	ErrorCode _CCONV backupAux(const StringParser & otherParser, const bool incremental, unsigned & amountBackup);
public:
	FTPStorage() : curl(nullptr), exportCurl(nullptr) {}
	ErrorCode _CCONV openStorage(const char * name) override;
	ErrorCode _CCONV add(const char * name, const char * data, const unsigned size) override;
	ErrorCode _CCONV get(const char * name, char ** data, unsigned * size) override;
	ErrorCode _CCONV exportFiles(const char * const * fileNames, const unsigned amount, const char * path) override;
	ErrorCode _CCONV backupFull(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV backupIncremental(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV remove(const char * name);
	~FTPStorage() override;
};


class MSSQLStorage : public Storage_impl
{
	void * henv; //SQLHANDLE
	void * hdbc; //HDBC
	void * hstmt_upsert, *hstmt_select, *hstmt_remove; //HSTMT

public:
	MSSQLStorage() : henv(nullptr), hdbc(nullptr), hstmt_upsert(nullptr), hstmt_select(nullptr), hstmt_remove(nullptr) {}
	ErrorCode _CCONV openStorage(const char * name) override;
	ErrorCode _CCONV add(const char * name, const char * data, const unsigned size) override;
	ErrorCode _CCONV get(const char * name, char ** data, unsigned * size) override;
	//ErrorCode _CCONV exportFiles(const char * const * fileNames, const unsigned amount, const char * path);//implemented in storage
	ErrorCode _CCONV backupFull(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV backupIncremental(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV remove(const char * name);
	~MSSQLStorage() override;
};

class PostgreSQLStorage : public Storage_impl
{
	void * conn; //PGconn
	std::string stmtInsert, stmtUpdate, stmtSelect, stmtRemove;
public:
	PostgreSQLStorage() : conn(nullptr) {}
	ErrorCode _CCONV openStorage(const char * name) override;
	ErrorCode _CCONV add(const char * name, const char * data, const unsigned size) override;
	ErrorCode _CCONV get(const char * name, char ** data, unsigned * size) override;
	//ErrorCode _CCONV exportFiles(const char * const * fileNames, const unsigned amount, const char * path);//implemented in storage
	ErrorCode _CCONV backupFull(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV backupIncremental(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV remove(const char * name);
	~PostgreSQLStorage() override;
};



struct sqlite3;
struct sqlite3_stmt;
//database and table names are necessary
class SQLiteStorage : public Storage_impl
{
	sqlite3 * db;
	sqlite3_stmt * pStmt_insert, *pStmt_update, *pStmt_select, *pStmt_remove;
public:
	SQLiteStorage() : db(nullptr), pStmt_insert(nullptr), pStmt_update(nullptr), pStmt_select(nullptr), pStmt_remove(nullptr) {}
	ErrorCode _CCONV openStorage(const char * name) override;
	ErrorCode _CCONV add(const char * name, const char * data, const unsigned size) override;
	ErrorCode _CCONV get(const char * name, char ** data, unsigned * size) override;
	//ErrorCode _CCONV exportFiles(const char * const * fileNames, const unsigned amount, const char * path);//implemented in storage
	ErrorCode _CCONV backupFull(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV backupIncremental(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV remove(const char * name);
	~SQLiteStorage() override;
};


class MongoDB : public Storage_impl
{
	void *client; //mongoc_client_t *
	void *collection; //mongoc_collection_t *
	void *fields; //bson_t *
public:
	MongoDB() : client(nullptr), collection(nullptr), fields(nullptr) {}
	ErrorCode _CCONV openStorage(const char * name) override;
	ErrorCode _CCONV add(const char * name, const char * data, const unsigned size) override;
	ErrorCode _CCONV get(const char * name, char ** data, unsigned * size) override;
	//ErrorCode _CCONV exportFiles(const char * const * fileNames, const unsigned amount, const char * path);//implemented in storage
	ErrorCode _CCONV backupFull(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV backupIncremental(const char * path, unsigned * amountChanged) override;
	ErrorCode _CCONV remove(const char * name);
	~MongoDB() override;
};