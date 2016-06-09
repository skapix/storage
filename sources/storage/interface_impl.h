#pragma once
#include "Storage.h"
#include "StringParser.h"
#include "registry_func.h"

struct Storage_impl : public Storage
{
protected:
	StringParser parser;
public:
	Storage_impl() {}
	virtual HRESULT _CCONV exportFiles(const char ** fileNames, const UINT amount, const char * path) override;//realized in storage.cpp
	virtual ~Storage_impl() {}
};

class FSBase : public Storage_impl
{
public:
	FSBase() {}
	HRESULT _CCONV add(const char * name, const char * data, const UINT size) override;
	HRESULT _CCONV get(const char * name, char ** data, UINT * size) override;
	//HRESULT _CCONV exportFiles(const char ** fileNames, const UINT amount, const char * path); // implemented in Storage_impl
	HRESULT _CCONV backupFull(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV backupIncremental(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV remove(const char * name);
	~FSBase() {};
};

class FSStorage : public FSBase
{
public:
	FSStorage() {}
	HRESULT _CCONV openStorage(const char * name) override;
	~FSStorage() override;
};

class SMBStorage : public FSBase
{
	void * nr; //LPNETRESOURCE
public:
	SMBStorage() : nr(nullptr) {}
	HRESULT _CCONV openStorage(const char * name) override;
	~SMBStorage() override;
};


//typedef void CURL;
class StringParser;
class FTPStorage : public Storage_impl
{
	void * curl, *exportCurl; //CURL
	HRESULT _CCONV backupAux(const StringParser & otherParser, const bool incremental, UINT & amountBackup);
public:
	FTPStorage() : curl(nullptr), exportCurl(nullptr) {}
	HRESULT _CCONV openStorage(const char * name) override;
	HRESULT _CCONV add(const char * name, const char * data, const UINT size) override;
	HRESULT _CCONV get(const char * name, char ** data, UINT * size) override;
	HRESULT _CCONV exportFiles(const char ** fileNames, const UINT amount, const char * path) override;
	HRESULT _CCONV backupFull(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV backupIncremental(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV remove(const char * name);
	~FTPStorage() override;
};


class MSSQLStorage : public Storage_impl
{
	void * henv; //SQLHANDLE
	void * hdbc; //HDBC
	void * hstmt_upsert, *hstmt_select, *hstmt_remove; //HSTMT

public:
	MSSQLStorage() : henv(nullptr), hdbc(nullptr), hstmt_upsert(nullptr), hstmt_select(nullptr), hstmt_remove(nullptr) {}
	HRESULT _CCONV openStorage(const char * name) override;
	HRESULT _CCONV add(const char * name, const char * data, const UINT size) override;
	HRESULT _CCONV get(const char * name, char ** data, UINT * size) override;
	//HRESULT _CCONV exportFiles(const char ** fileNames, const UINT amount, const char * path);//implemented in storage
	HRESULT _CCONV backupFull(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV backupIncremental(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV remove(const char * name);
	~MSSQLStorage() override;
};

class PostgreSQLStorage : public Storage_impl
{
	void * conn; //PGconn
	std::string stmtInsert, stmtUpdate, stmtSelect, stmtRemove;
public:
	PostgreSQLStorage() : conn(nullptr) {}
	HRESULT _CCONV openStorage(const char * name) override;
	HRESULT _CCONV add(const char * name, const char * data, const UINT size) override;
	HRESULT _CCONV get(const char * name, char ** data, UINT * size) override;
	//HRESULT _CCONV exportFiles(const char ** fileNames, const UINT amount, const char * path);//implemented in storage
	HRESULT _CCONV backupFull(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV backupIncremental(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV remove(const char * name);
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
	HRESULT _CCONV openStorage(const char * name) override;
	HRESULT _CCONV add(const char * name, const char * data, const UINT size) override;
	HRESULT _CCONV get(const char * name, char ** data, UINT * size) override;
	//HRESULT _CCONV exportFiles(const char ** fileNames, const UINT amount, const char * path);//implemented in storage
	HRESULT _CCONV backupFull(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV backupIncremental(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV remove(const char * name);
	~SQLiteStorage() override;
};


class MongoDB : public Storage_impl
{
	void *client; //mongoc_client_t *
	void *collection; //mongoc_collection_t *
	void *fields; //bson_t *
public:
	MongoDB() : client(nullptr), collection(nullptr), fields(nullptr) {}
	HRESULT _CCONV openStorage(const char * name) override;
	HRESULT _CCONV add(const char * name, const char * data, const UINT size) override;
	HRESULT _CCONV get(const char * name, char ** data, UINT * size) override;
	//HRESULT _CCONV exportFiles(const char ** fileNames, const UINT amount, const char * path);//implemented in storage
	HRESULT _CCONV backupFull(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV backupIncremental(const char * path, UINT * amountChanged) override;
	HRESULT _CCONV remove(const char * name);
	~MongoDB() override;
};