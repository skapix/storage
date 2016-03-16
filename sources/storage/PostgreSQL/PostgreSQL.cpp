#include "..\\interface_impl.h"
#include "libpq-fe.h"
#include "..\\auxiliaryStorage.h"
#include <boost/version.hpp> 
#if defined(_MSC_VER) && BOOST_VERSION==105700 
//#pragma warning(disable:4003) 
#define BOOST_PP_VARIADICS 0 
#endif
#include "boost\scope_exit.hpp"


using std::string;

const char g_SQL_CREATE[] = "CREATE TABLE IF NOT EXISTS %s(name TEXT PRIMARY KEY,text bytea,stamp TIMESTAMP DEFAULT(CURRENT_TIMESTAMP));";//blob ~ bytea //in postgresql
const char g_SQL_UPDATE[] = "UPDATE %s SET text = $2::bytea, stamp = CURRENT_TIMESTAMP where name = $1::text";
const char g_SQL_INSERT[] = "INSERT INTO %s(name, text) VALUES($1::text, $2::bytea)";
const char g_SQL_SELECT[] = "SELECT text FROM %s WHERE name = $1::text";
const char g_SQL_REMOVE[] = "delete from %s where name = $1::text";
const char g_SQL_FULL_INC[] = "SELECT * FROM %s WHERE stamp > $1::timestamp";


const char g_prepared_insert[] = "_insertproc|";
const char g_prepared_update[] = "_updateproc|";
const char g_prepared_select[] = "_selectproc|";
const char g_prepared_remove[] = "_removeproc|";



bool createStmtProc(PGconn * conn, const char * unbindedQuery, const string & tablename,
	const char * stmtOrig, const unsigned amountParams, string & stmtFinal)
{
	string query;
	bindParam(tablename, unbindedQuery, query);
	stmtFinal = string(stmtOrig) + tablename;
	PGresult * res = PQprepare(conn, stmtFinal.c_str(), query.c_str(), amountParams, NULL);
	bool retVal = PQresultStatus(res) == PGRES_COMMAND_OK;
	PQclear(res);
	return retVal;
}

HRESULT _CCONV PostgreSQLStorage::openStorage(const char * dataPath)
{
	if (dataPath == nullptr || !parser.initialize(dataPath))
		return E_INVALIDARG;
	PGresult * res;
	const string table = parser.getTableName(), database = parser.getDbName();
	if (table.empty() || database.empty())
		return E_INVALIDARG;
	const char * port = parser.getPort().empty() ? nullptr : parser.getPort().c_str();
	conn = PQsetdbLogin(parser.getServer().c_str(), port,
		NULL, NULL, database.c_str(), parser.getLogin().c_str(), parser.getPass().c_str());
	if (conn == nullptr)
		return E_INVALIDARG;
	string sql_create;
	bindParam(table, g_SQL_CREATE, sql_create);
	res = PQexec((PGconn*)conn, sql_create.c_str());
	BOOST_SCOPE_EXIT(res) { PQclear(res); } BOOST_SCOPE_EXIT_END
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
		return CO_E_FAILEDTOGETWINDIR;

	const char * sqlQueries[] = { g_SQL_INSERT, g_SQL_UPDATE, g_SQL_SELECT, g_SQL_REMOVE };
	const char * sqlProcNamesOrig[] = { g_prepared_insert, g_prepared_update, g_prepared_select, g_prepared_remove };
	string * sqlProc[] = { &stmtInsert, &stmtUpdate, &stmtSelect, &stmtRemove };
	const int numParams[] = { 2, 2, 1, 1 };
	for (size_t i = 0; i < 3; i++)
	{
		if (!createStmtProc((PGconn*)conn, sqlQueries[i], table, sqlProcNamesOrig[i], numParams[i], *sqlProc[i]))
			return E_UNEXPECTED;
	}
	return S_OK;
}


HRESULT _CCONV PostgreSQLStorage::add(const char * name, const char * data, const UINT size)
{
	if (name == nullptr || data == nullptr)
		return E_INVALIDARG;
	const char * const args[] = { name, data };
	const int paramLength[] = { strlen(name), size };
	const int paramFormats[] = { 0, 1 };//text,binary
	PGresult * res = PQexecPrepared((PGconn*)conn, stmtInsert.c_str(), 2, args, paramLength, paramFormats, 0);
	if (PQresultStatus(res) == PGRES_FATAL_ERROR)	//file already exists => rewrite it
		res = PQexecPrepared((PGconn*)conn, stmtUpdate.c_str(), 2, args, paramLength, paramFormats, 0);
	HRESULT retVal = PQresultStatus(res) == PGRES_COMMAND_OK ? S_OK : E_FAIL;
	PQclear(res);
	return retVal;
}

inline void postgreGetValue(PGresult * res, const int tup_num, const int field_num, string & data)
{
	int resLength = PQgetlength(res, tup_num, field_num);
	data.resize(resLength);
	memcpy(&data[0], PQgetvalue(res, tup_num, field_num), resLength);
}

HRESULT _CCONV PostgreSQLStorage::get(const char * name, char ** data, UINT * size)
{
	if (name == nullptr || size == nullptr)
		return E_INVALIDARG;
	const char * const args[] = { name };
	const int paramLength[] = { strlen(name) };
	PGresult * res = PQexecPrepared((PGconn*)conn, stmtSelect.c_str(), 1, args, paramLength, 0, 1); //0 ~ all pars are text, 1 ~ binary output
	BOOST_SCOPE_EXIT(res) { PQclear(res); } BOOST_SCOPE_EXIT_END
	if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
		return S_FALSE;
	UINT resLength = PQgetlength(res, 0, 0);
	d_getFunc(data, size, resLength, memcpy(*data, PQgetvalue(res, 0, 0), resLength););
	return S_OK;
}



//start backup
///////////////////////////////////////////////////////////////////////

const char g_SQL_CREATEDATE[] = "CREATE TABLE IF NOT EXISTS _syslastbackupinc\
								(fromPath TEXT, toPath TEXT, stamp TIMESTAMP, constraint _syspk primary key(fromPath,toPath))";

const char g_SQL_INSERTDATE[] = "INSERT INTO _syslastbackupinc(stamp,fromPath,toPath) VALUES('%s','%s','%s')";
const char g_SQL_UPDATEDATE[] = "UPDATE _syslastbackupinc SET stamp = '%s' where fromPath = '%s' and toPath = '%s'";
const char g_SQL_GETCURDATE[] = "select CURRENT_TIMESTAMP";
const char g_SQL_SELECTLASTDATE[] = "SELECT stamp from _syslastbackupinc where fromPath = $1::text and toPath = $2::text";

const char g_SQL_SELECTCORTEGE[] = "SELECT * from %s where stamp > $1::timestamp";
const char g_SQL_INSERTVALUES[] = "INSERT into %s(text, stamp, name) values($1::bytea,$2::timestamp,$3::text)";
const char g_SQL_UPDATEVALUES[] = "UPDATE %s SET text = $1::bytea,stamp = $2::timestamp where name = $3::text";



inline bool postgreUpsert(PGconn * conn, const char * query_insert, const char * query_update,
	const int nParams, const char * const * args, const int * paramLength, const int * paramFormats)
{
	
	PGresult * res = PQexecPrepared(conn, query_insert, nParams, args, paramLength, paramFormats, 0);
	if (PQresultStatus(res) == PGRES_FATAL_ERROR)
		res = PQexecPrepared(conn, query_update, nParams, args, paramLength, paramFormats, 0);
	bool retVal = PQresultStatus(res) == PGRES_COMMAND_OK;
	PQclear(res);
	return retVal;
}



HRESULT backupAux(PGconn * fromConn, const string & fromPath, PGconn * toConn, const string & toPath,
	const string & date, UINT & amountBackup)
{
	//create table in db
	string sqlCreate;
	bindParam(toPath, g_SQL_CREATE, sqlCreate);
	PGresult * res = PQexec((PGconn*)toConn, sqlCreate.c_str());
	BOOST_SCOPE_EXIT(res) { PQclear(res); } BOOST_SCOPE_EXIT_END
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
		return E_FAIL;

	string sqlSelect;
	bindParam(fromPath, g_SQL_SELECTCORTEGE, sqlSelect);
	//select query from conn
	int paramLength = static_cast<int>(date.size());
	const char * const param[] = { date.c_str() };
	res = PQexecParams(fromConn, sqlSelect.c_str(), 1, NULL, param, &paramLength, NULL, 1);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return E_FAIL;
		
	amountBackup = PQntuples(res);
	// read name, text, stamp
	//write text, stamp, name
	const char g_prepared_insertWithDate[] = "_insertwithdate|";
	const char g_prepared_updateWithDate[] = "_updatewithdate|";
	
	string stmtInsert, stmtUpdate;
	if (!createStmtProc(toConn, g_SQL_INSERTVALUES, toPath, g_prepared_insertWithDate, 3, stmtInsert) ||
		!createStmtProc(toConn, g_SQL_UPDATEVALUES, toPath, g_prepared_updateWithDate, 3, stmtUpdate))
		return E_UNEXPECTED;
	for (UINT i = 0; i < amountBackup; i++)
	{
		string name, text, stamp;
		postgreGetValue(res, i, 0, name);
		postgreGetValue(res, i, 1, text);
		postgreGetValue(res, i, 2, stamp);
		const char * const args[] = { text.c_str(), stamp.c_str(), name.c_str() };
		const int paramLength[] = { text.size(), stamp.size(), name.size() };
		const int paramFormats[] = { 1, 1, 0 };//binary,binary,text
		if (!postgreUpsert(toConn, stmtInsert.c_str(), stmtUpdate.c_str(), 3, args, paramLength, paramFormats))
			return E_FAIL;
	}
	return S_OK;
}

////////////////////////////////////
//backupFull


//2 types of full backup
//1 : copy table to 'aux.dat'
//    copy table2 from 'aux.dat'
//2 : insert into table2 select * from table
HRESULT _CCONV PostgreSQLStorage::backupFull(const char * path, UINT * amountChanged)
{
	if (path == nullptr)
		return E_INVALIDARG;
	StringParser otherParser(path);
	PGconn * newConn = PQsetdbLogin(otherParser.getServer().c_str(), otherParser.getPort().c_str(), NULL,
		NULL, otherParser.getDbName().c_str(), otherParser.getLogin().c_str(), otherParser.getPass().c_str());
	if (!newConn)
		return E_INVALIDARG;
	UINT amountBackup = 0;
	HRESULT retVal = backupAux((PGconn*)conn,parser.getTableName(),newConn,otherParser.getTableName(), "-infinity", amountBackup);
	PQfinish(newConn);
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return retVal;
}


bool selectNowDate(PGconn * conn, string & date)
{
	PGresult * res = PQexec(conn, g_SQL_GETCURDATE);
	if (PQntuples(res) != 1)
		return false;
	int resLength = PQgetlength(res, 0, 0);
	date.resize(resLength);
	memcpy(&date[0], PQgetvalue(res, 0, 0), resLength);
	return true;
}

bool selectLastDate(PGconn * conn, const string & fromPath, const string & toPath, string & date)
{
	const char * const param[] = { fromPath.c_str(), toPath.c_str() };
	int paramLength[] = { static_cast<int>(fromPath.size()), static_cast<int>(toPath.size()) };

	PGresult * res = PQexecParams(conn, g_SQL_SELECTLASTDATE, 2, NULL, param, paramLength, 0, 0);
	if (PQntuples(res)>0)
	{
		int resLength = PQgetlength(res, 0, 0);
		date.resize(resLength);
		memcpy(&date[0], PQgetvalue(res, 0, 0), resLength);
	}
	else
		date.assign("-infinity");
	bool retVal = PQresultStatus(res) == PGRES_TUPLES_OK;
	PQclear(res);
	return retVal;
}

bool upsertNowDate(PGconn * conn, const string & date, const string & fromPath, const string & toPath)
{
	string query;
	bind3Params(date, fromPath, toPath, g_SQL_INSERTDATE, query);
	PGresult * res = PQexec(conn, query.c_str());
	if (PQresultStatus(res) == PGRES_FATAL_ERROR)
	{
		bind3Params(date, fromPath, toPath, g_SQL_UPDATEDATE, query);
		res = PQexec(conn, query.c_str());
	}
	bool retVal = PQresultStatus(res) == PGRES_COMMAND_OK;
	PQclear(res);
	return retVal;
}

inline bool clearExit(PGconn * conn, PGresult * res, bool retVal)
{
	PQclear(res);
	PQfinish(conn);
	return retVal;
}

HRESULT _CCONV PostgreSQLStorage::backupIncremental(const char * path, UINT * amountChanged)
{
	if (path == nullptr)
		return E_INVALIDARG;
	//create table in dst db
	StringParser otherParser(path);

	PGconn * newConn = PQsetdbLogin(otherParser.getServer().c_str(), otherParser.getPort().c_str(), NULL,
		NULL, otherParser.getDbName().c_str(), otherParser.getLogin().c_str(), otherParser.getPass().c_str());
	if (!newConn)
		return E_INVALIDARG;
	BOOST_SCOPE_EXIT(newConn) { PQfinish(newConn); } BOOST_SCOPE_EXIT_END;
	//create auxiliary db (if not exists) with time
	PGresult * res = PQexec(newConn, g_SQL_CREATEDATE);
	BOOST_SCOPE_EXIT(res) { PQclear(res); } BOOST_SCOPE_EXIT_END;
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
		return E_FAIL;
	//select last timestamp, now timestamp
	string lastDate, nowDate;
	if (!selectLastDate(newConn, parser.getTableName(), otherParser.getTableName(), lastDate) ||
		!selectNowDate(newConn, nowDate))
		return E_FAIL;
	UINT amountBackup = 0;
	HRESULT retVal = backupAux((PGconn*)conn, parser.getTableName(), newConn, otherParser.getTableName(), lastDate, amountBackup);
	if (SUCCEEDED(retVal))
		retVal = upsertNowDate(newConn, nowDate, parser.getTableName(), otherParser.getTableName()) ? S_OK : E_FAIL;
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return retVal;
}


HRESULT _CCONV PostgreSQLStorage::remove(const char * name)
{
	if (name == nullptr)
		return E_INVALIDARG;
	const char * const args[] = { name };
	const int paramLength[] = { strlen(name) };
	PGresult * res = PQexecPrepared((PGconn*)conn, stmtRemove.c_str(), 1, args, paramLength, 0, 0);
	HRESULT retVal = PQresultStatus(res) == PGRES_COMMAND_OK ? S_OK : S_FALSE;
	PQclear(res);
	return retVal;

}

PostgreSQLStorage::~PostgreSQLStorage()
{
	if (conn)
		PQfinish((PGconn*)conn);
}
