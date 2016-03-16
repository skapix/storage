#include "..\\interface_impl.h"
#include "sqlite3.h"
#include "..\\auxiliaryStorage.h"
#include <boost/version.hpp> 
#if defined(_MSC_VER) && BOOST_VERSION==105700 
//#pragma warning(disable:4003) 
#define BOOST_PP_VARIADICS 0 
#endif
#include "boost\scope_exit.hpp"

using namespace std;



const char g_SQL_CREATE[] = "CREATE TABLE IF NOT EXISTS %s(name TEXT PRIMARY KEY,text BLOB NOT NULL,stamp DATE DEFAULT (datetime('now','localtime')))";
const char g_SQL_UPDATE[] = "UPDATE %s SET text = ?, stamp = datetime('now','localtime') where name = ?";
const char g_SQL_INSERT[] = "INSERT INTO %s(text, name) VALUES(?, ?)";
const char g_SQL_SELECT[] = "SELECT text FROM %s WHERE name = ?";
const char g_SQL_REMOVE[] = "delete from %s where name = ?";

HRESULT _CCONV SQLiteStorage::openStorage(const char * st)
{
	if (st == nullptr || !parser.initialize(st))
		return E_INVALIDARG;
	if (parser.getTableName().empty() || parser.getDbName().empty())
		return E_INVALIDARG;

	string createQuery;
	bindParam(parser.getTableName(), g_SQL_CREATE, createQuery);
	int err;

	if (SQLITE_OK != (err = sqlite3_open(parser.getDbName().c_str(), &db)))
		return E_FAIL;
	else if (SQLITE_OK != (err = sqlite3_exec(db, createQuery.c_str(), 0, 0, nullptr)))
		return CO_E_FAILEDTOGETWINDIR;

	const char * sqlQueryUnbinded[] = { g_SQL_SELECT, g_SQL_INSERT, g_SQL_UPDATE, g_SQL_REMOVE };
	sqlite3_stmt ** pStmtParam[] = { &pStmt_select, &pStmt_insert, &pStmt_update, &pStmt_remove };
	for (size_t i = 0; i < 4; i++)
	{
		string sqlQuery;
		bindParam(parser.getTableName(), sqlQueryUnbinded[i], sqlQuery);
		err = sqlite3_prepare_v2(db, sqlQuery.c_str(), sqlQuery.size(), pStmtParam[i], NULL);
		if (err != SQLITE_OK)
			return E_UNEXPECTED;
	}
	return S_OK;
}


//BT ~ blob text
int bindAndExecBT(sqlite3_stmt * pStmt, const string & blob, const string & text)
{

	sqlite3_bind_blob(pStmt, 1, blob.data(), blob.size(), SQLITE_STATIC);
	sqlite3_bind_text(pStmt, 2, text.c_str(), text.length(), SQLITE_STATIC);
	int rc = sqlite3_step(pStmt);
	sqlite3_reset(pStmt);
	return rc;
}

int bindAndExecBT(sqlite3_stmt * pStmt, const char * blob, const UINT size, const char * text)
{

	sqlite3_bind_blob(pStmt, 1, blob, size, SQLITE_STATIC);
	sqlite3_bind_text(pStmt, 2, text, strlen(text), SQLITE_STATIC);
	int rc = sqlite3_step(pStmt);
	sqlite3_reset(pStmt);
	return rc;
}

HRESULT _CCONV SQLiteStorage::add(const char * name, const char * data, const UINT size)
{
	int retVal = bindAndExecBT(pStmt_insert, data, size, name);
	if (retVal == SQLITE_CONSTRAINT)
		retVal = bindAndExecBT(pStmt_update, data, size, name);
	return retVal == SQLITE_DONE ? S_OK : E_FAIL;
}


HRESULT _CCONV SQLiteStorage::get(const char * name, char ** data, UINT * size)
{
	sqlite3_bind_text(pStmt_select, 1, name, strlen(name), SQLITE_STATIC);
	BOOST_SCOPE_EXIT(pStmt_select) { sqlite3_reset(pStmt_select); } BOOST_SCOPE_EXIT_END;
	int rc = sqlite3_step(pStmt_select);
	if (rc == SQLITE_ROW)
	{
		UINT length = sqlite3_column_bytes(pStmt_select, 0);
		d_getFunc(data, size, length, memcpy(*data, sqlite3_column_blob(pStmt_select, 0), length));
		return sqlite3_step(pStmt_select) == SQLITE_DONE ? S_OK : E_FAIL;
	}
	else
		return S_FALSE;
	//sqlite3_clear_bindings(pStmt_select);
}

///////////////////////////////////
//backup

const char g_SQL_SELECTCORTEGE[] = "SELECT * from %s where stamp > '%s'";
const char g_SQL_INSERTVALUES[] = "INSERT into %s(text, stamp, name) values(?,?,?)";
const char g_SQL_UPDATEVALUES[] = "UPDATE %s SET text = ?,stamp = ? where name = ?";


int bindAndExecBTT(sqlite3_stmt *pStmt, const void * data,
	const int dataLength, const char * stamp, const char * name)
{
	int rc;
	sqlite3_bind_blob(pStmt, 1, data, dataLength, SQLITE_STATIC);
	sqlite3_bind_text(pStmt, 2, stamp, -1, SQLITE_STATIC);
	sqlite3_bind_text(pStmt, 3, name, -1, SQLITE_STATIC);
	rc = sqlite3_step(pStmt);
	sqlite3_reset(pStmt);
	return rc;
}

HRESULT sqliteBackup(sqlite3 * pOurDb, const string & ourTablename, sqlite3* pDestDb, const string & destTablename, 
	const string & date, UINT & amountBackup)
{
	//binding params
	string sql_selectText;
	bind2Params(ourTablename, date, g_SQL_SELECTCORTEGE, sql_selectText);

	string insertQuery, updateQuery;
	bindParam(destTablename, g_SQL_INSERTVALUES, insertQuery);
	bindParam(destTablename, g_SQL_UPDATEVALUES, updateQuery);

	//create table in destDb
	string createQuery;
	bindParam(destTablename, g_SQL_CREATE, createQuery);
	int err;
	if (SQLITE_OK != sqlite3_exec(pDestDb, createQuery.c_str(), 0, 0, nullptr))
		return E_FAIL;
	//prepared statements
	sqlite3_stmt * pStmtInsert, *pStmtUpdate, *pStmtSelect;
	err = sqlite3_prepare_v2(pDestDb, insertQuery.c_str(), insertQuery.length(), &pStmtInsert, 0);
	if (err != SQLITE_OK)
		return E_UNEXPECTED;
	BOOST_SCOPE_EXIT(pStmtInsert) { sqlite3_finalize(pStmtInsert); } BOOST_SCOPE_EXIT_END;
	err = sqlite3_prepare_v2(pDestDb, updateQuery.c_str(), updateQuery.length(), &pStmtUpdate, 0);
	if (err != SQLITE_OK)
		return E_UNEXPECTED;
	BOOST_SCOPE_EXIT(pStmtUpdate) { sqlite3_finalize(pStmtUpdate); } BOOST_SCOPE_EXIT_END;
	err = sqlite3_prepare_v2(pOurDb, sql_selectText.c_str(), -1, &pStmtSelect, 0);
	if (err != SQLITE_OK)
		return E_UNEXPECTED;
	BOOST_SCOPE_EXIT(pStmtSelect) { sqlite3_finalize(pStmtSelect); } BOOST_SCOPE_EXIT_END;

	//get and paste values
	while ((err = sqlite3_step(pStmtSelect)) == SQLITE_ROW)
	{
		const char * name = (const char*)sqlite3_column_text(pStmtSelect, 0),
			*timestamp = (const char*)sqlite3_column_text(pStmtSelect, 2);
		const void * blob = sqlite3_column_blob(pStmtSelect, 1);
		const int blobSize = sqlite3_column_bytes(pStmtSelect, 1);
		//inserting values
		int retVal = bindAndExecBTT(pStmtInsert, blob, blobSize, timestamp, name);
		if (retVal == SQLITE_CONSTRAINT)
			retVal = bindAndExecBTT(pStmtUpdate, blob, blobSize, timestamp, name);
		//inserting is not ok
		if (retVal != SQLITE_DONE)
			return E_FAIL;
		++amountBackup;
	}
	return err == SQLITE_DONE ? S_OK : E_FAIL;
	
}


HRESULT _CCONV SQLiteStorage::backupFull(const char * path, UINT * amountChanged)
{
	if (path == nullptr)
		return E_INVALIDARG;
	StringParser otherParser(path);
	if (parser.getTableName().empty() || parser.getDbName().empty())
		return E_INVALIDARG;
	sqlite3 * pDest;
	if (parser.getDbName() == otherParser.getDbName())
		pDest = db;
	else
	{
		int res = sqlite3_open(otherParser.getDbName().c_str(), &pDest);
		if (res != SQLITE_OK)
			return CO_E_FAILEDTOGETWINDIR;
	}
	UINT amountBackup = 0;
	HRESULT retVal = sqliteBackup(db, parser.getTableName(), pDest, otherParser.getTableName(), string(""), amountBackup);
	if (pDest != db)
		sqlite3_close(pDest);
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return retVal;
}

//start backupIncremental
///////////////////////////////////////////////////////////////////////
const char g_SQL_CREATEDATE[] = "CREATE TABLE IF NOT EXISTS _syslastbackupinc\
								(fromPath TEXT, toPath TEXT, stamp DATE, constraint _syspk primary key(fromPath,toPath))";
const char g_SQL_INSERTDATE[] = "INSERT INTO _syslastbackupinc(stamp, fromPath, toPath) VALUES('%s','%s','%s')";
const char g_SQL_UPDATEDATE[] = "UPDATE _syslastbackupinc SET stamp = '%s' where fromPath = '%s' and toPath = '%s'";
const char g_SQL_GETCURDATE[] = "select datetime('now','localtime')";
const char g_SQL_SELECTLASTDATE[] = "SELECT stamp from _syslastbackupinc where fromPath = '%s' and toPath = '%s'";


//lastDate ~ last date in log file
//!lastDate ~ now
int callbackSelectTextFromQuery(void * param, int numColumns, char ** columnText, char ** columnName)
{
	assert(numColumns == 1);
	string * st = (string*)param;
	st->assign(columnText[0], strlen(columnText[0]));
	return 1;//false
}

bool selectSingleTextFromQuery(sqlite3 * pDestDb, string & text, const char * query)
{
	int err = sqlite3_exec(pDestDb, query, callbackSelectTextFromQuery, &text, nullptr);
	if (err != SQLITE_OK && err != SQLITE_ABORT)//ok ~ no data; abort ~ have data
		return false;
	return true;
}


int substituteAndExecTTT(const char * sqlReq, sqlite3 * db, const string & t1, const string & t2, const string & t3)
{
	string query;
	bind3Params(t1, t2, t3, sqlReq, query);
	return sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
}



//whole database backup
//err = sqlite3_exec(this->db, "SELECT name FROM sqlite_master WHERE type='table'", backup_callback, &arg, &errMsg);


HRESULT SQLiteStorage::backupIncremental(const char * path, UINT * amountChanged)
{
	if (path == nullptr)
		return E_INVALIDARG;
	StringParser otherParser(path);
	if (otherParser.getTableName().empty() || otherParser.getDbName().empty())
		return E_INVALIDARG;
	sqlite3 * pDest;
	int err;
	
	if (parser.getDbName() == otherParser.getDbName())
	{
		pDest = db;
	}
	else
	{
		err = sqlite3_open(otherParser.getDbName().c_str(), &pDest);
		if (err != SQLITE_OK)
			return CO_E_FAILEDTOGETWINDIR;
	}
	BOOST_SCOPE_EXIT(pDest, db) { if (pDest != db) sqlite3_close(pDest); } BOOST_SCOPE_EXIT_END;
	if (SQLITE_OK != (err = sqlite3_exec(pDest, g_SQL_CREATEDATE, 0, 0, nullptr)))
		return E_FAIL;
	

	string nowDate, lastDate;
	string newQuery;
	bind2Params(parser.getTableName(), otherParser.getTableName(), g_SQL_SELECTLASTDATE, newQuery);
	if (!selectSingleTextFromQuery(pDest, lastDate, newQuery.c_str()) ||
		!selectSingleTextFromQuery(pDest, nowDate, g_SQL_GETCURDATE))
		return E_FAIL;
	UINT amountBackup = 0;
	HRESULT res = sqliteBackup(db, parser.getTableName(), pDest, otherParser.getTableName(), lastDate, amountBackup);
	if (FAILED(res))
		return res;
	//insertDate
	err = substituteAndExecTTT(g_SQL_INSERTDATE, pDest, nowDate, parser.getTableName(), otherParser.getTableName());
	if (err == SQLITE_CONSTRAINT)
		err = substituteAndExecTTT(g_SQL_UPDATEDATE, pDest, nowDate, parser.getTableName(), otherParser.getTableName());
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return err == SQLITE_DONE || err == SQLITE_OK ? S_OK : E_FAIL;
}

HRESULT _CCONV SQLiteStorage::remove(const char * name)
{
	if (name == nullptr)
		return E_INVALIDARG;
	sqlite3_bind_text(pStmt_remove, 1, name, strlen(name), SQLITE_STATIC);
	int rc = sqlite3_step(pStmt_remove);
	sqlite3_reset(pStmt_remove);
	return rc == SQLITE_OK ? S_OK : S_FALSE;
}

SQLiteStorage::~SQLiteStorage()
{
	if (pStmt_insert)
		sqlite3_finalize(pStmt_insert);
	if (pStmt_select)
		sqlite3_finalize(pStmt_select);
	if (pStmt_update)
		sqlite3_finalize(pStmt_update);
	if (pStmt_remove)
		sqlite3_finalize(pStmt_remove);
	if (db)
		sqlite3_close(db);
}