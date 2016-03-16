#include "..\\interface_impl.h"
#include "..\\auxiliaryStorage.h"
#include <sql.h>
#include <sqlext.h>
#include <boost/version.hpp> 
#if defined(_MSC_VER) && BOOST_VERSION==105700 
//#pragma warning(disable:4003) 
#define BOOST_PP_VARIADICS 0 
#endif
#include <boost\scope_exit.hpp>


using namespace std;

const SQLCHAR g_SQL_CREATE[] = "IF OBJECT_ID('%s') IS NULL CREATE TABLE %s(name VARCHAR(250) PRIMARY KEY, "
						    "value VARBINARY(max), stamp DATETIME2(7) DEFAULT CURRENT_TIMESTAMP)";

const SQLCHAR g_SQL_UPSERT[] = "merge %s as target "
								"using (values(?, ?)) "
								"as source(field1, field2) "
								"on target.name = source.field1 "
								"when matched then "
								"update set target.value = source.field2, target.stamp = CURRENT_TIMESTAMP "
								"when not matched then "
								"insert (name,value) values(source.field1, source.field2);";

const SQLCHAR g_SQL_SELECT[] = "select value from %s where name = ?";

const SQLCHAR g_SQL_REMOVE[] = "delete from %s where name = ?";

const size_t g_maxColumnNameSize = 250;
const size_t g_maxColumnValueSize = g_maxFileSize;

#define MYSQLSUCCESS(rc) ( (rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO) )
#define check_rc(rc,E_ERR) if (!MYSQLSUCCESS(rc)) return E_ERR;

//FOR DEBUG
//handle   : //henb			  //hdbc           //stmt            //desc
//smallInt : //SQL_HANDLE_ENV //SQL_HANDLE_DBC //SQL_HANDLE_STMT //SQL_HANDLE_DESC
SQLRETURN printError(SQLHANDLE handle, SQLSMALLINT smallInt)
{	
	//SQL_HANDLE_DBC
	// String to hold the SQL State
	SQLCHAR szSQLSTATE[10];
	// Error code
	SDWORD nErr;
	// The error message
	SQLCHAR msg[SQL_MAX_MESSAGE_LENGTH + 1];
	// Size of the message
	SWORD cbmsg;
	SQLRETURN ret = SQLGetDiagRec(smallInt, handle, 1, szSQLSTATE, &nErr, msg, sizeof(msg), &cbmsg);
	printf("%s", msg);
	return ret;
}

SQLRETURN connect(const SQLHANDLE hdbc, SQLCHAR * szDSN)
{
	const unsigned MAX_DATA = 1024;
	SQLCHAR szConnStrOut[MAX_DATA + 1];
	SWORD swStrLen;
	return SQLDriverConnect(hdbc, NULL, szDSN, SQL_NTS, szConnStrOut, MAX_DATA, &swStrLen, SQL_DRIVER_NOPROMPT);
}

SQLRETURN connectToDb(HENV henv, const StringParser & parser, HDBC & hdbc)
{
	SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	check_rc(retcode, retcode);
	//for massive copy
	//retcode = SQLSetConnectAttr(hdbc, SQL_COPT_SS_BCP, (void *)SQL_BCP_ON, SQL_IS_INTEGER);
	//check_rc(retcode, "SQLSetConnectAttr(hdbc1) Failed\n\n");
	string szDSN = "Driver={SQL Server Native Client 11.0};Server=" + parser.getServer() + ";Database=" +
		parser.getDbName() + ";UID=" + parser.getLogin() + ";PWD=" + parser.getPass();
	retcode = connect(hdbc, (SQLCHAR*)szDSN.c_str());
	if (!MYSQLSUCCESS(retcode))
	{
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		return retcode;
	}
	//retcode = SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)TRUE, 0);
	return retcode;
}

HRESULT _CCONV MSSQLStorage::openStorage(const char * dataPath)
{
	if (dataPath == nullptr)
		return E_INVALIDARG;
	parser.initialize(dataPath);
	SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_ENV, NULL, &henv);
	check_rc(retcode, E_UNEXPECTED);
	retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER);
	check_rc(retcode, E_UNEXPECTED);
	retcode = connectToDb(henv, parser, hdbc);
	check_rc(retcode, E_FAIL);
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt_select);
	check_rc(retcode, E_UNEXPECTED);
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt_upsert);
	check_rc(retcode, E_UNEXPECTED);
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt_remove);
	check_rc(retcode, E_UNEXPECTED);
//creating table (if not exists)
	string sql_createDb;
	bind2Params(parser.getTableName(), parser.getTableName(), (const char *)g_SQL_CREATE, sql_createDb);
	retcode = SQLExecDirect(hstmt_select, (SQLCHAR *)sql_createDb.c_str(), SQL_NTS);
	check_rc(retcode, CO_E_FAILEDTOGETWINDIR);//can't create table
	SQLFreeStmt(hstmt_select, SQL_CLOSE);
	string sql_upsert, sql_select, sql_remove;
	bindParam(parser.getTableName(), (const char *)g_SQL_UPSERT, sql_upsert);
	bindParam(parser.getTableName(), (const char *)g_SQL_SELECT, sql_select);
	bindParam(parser.getTableName(), (const char *)g_SQL_SELECT, sql_remove);
	//SQLSetStmtAttr(hstmt, SQL_ATTR_ASYNC_ENABLE, (SQLPOINTER)SQL_ASYNC_ENABLE_ON, 0);
	retcode = SQLPrepare(hstmt_upsert, (SQLCHAR*)sql_upsert.c_str(), SQL_NTS);
	check_rc(retcode, E_UNEXPECTED);
	retcode = SQLPrepare(hstmt_select, (SQLCHAR*)sql_select.c_str(), SQL_NTS);
	check_rc(retcode, E_UNEXPECTED);
	retcode = SQLPrepare(hstmt_remove, (SQLCHAR*)sql_remove.c_str(), SQL_NTS);
	check_rc(retcode, E_UNEXPECTED);
	
	return S_OK;
}

HRESULT _CCONV MSSQLStorage::add(const char * name, const char * data, const UINT size)
{
	if (name == nullptr || data == nullptr)
		return E_INVALIDARG;
	SQLRETURN rc = SQLBindParameter(hstmt_upsert, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, g_maxColumnNameSize, 0,
		(SQLPOINTER*)name, strlen(name), NULL);
	check_rc(rc, E_UNEXPECTED);
	BOOST_SCOPE_EXIT(hstmt_upsert){ SQLFreeStmt(hstmt_upsert, SQL_CLOSE); } BOOST_SCOPE_EXIT_END;
	
	
	SQLLEN iDataLength = size;
	rc = SQLBindParameter(hstmt_upsert, 2, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, g_maxColumnValueSize, 0,
		(SQLPOINTER*)data, size, &iDataLength);
	check_rc(rc, E_UNEXPECTED);
	rc = SQLExecute(hstmt_upsert);

	return MYSQLSUCCESS(rc) ? S_OK : E_FAIL;
}

HRESULT _CCONV MSSQLStorage::get(const char * name, char ** data, UINT * size)
{
	if (name == nullptr || size == nullptr)
		return E_INVALIDARG;
	SQLRETURN rc = SQLBindParameter(hstmt_select, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 250, 0,
		(SQLPOINTER*)name, strlen(name), NULL);
	check_rc(rc, E_UNEXPECTED);
	BOOST_SCOPE_EXIT(hstmt_select){ SQLFreeStmt(hstmt_select, SQL_CLOSE); } BOOST_SCOPE_EXIT_END;
	
	rc = SQLExecute(hstmt_select);
	check_rc(rc, E_FAIL);
	
	rc = SQLFetch(hstmt_select);
	check_rc(rc, S_FALSE);
	char buf[1];//TargetValue can not be NULL
	SQLINTEGER pIndicator;
	
	if (SQLGetData(hstmt_select, 1, SQL_C_BINARY, buf, 0, &pIndicator) == SQL_SUCCESS_WITH_INFO)
	{
		
		d_getFunc(data, size, (ULONG)pIndicator, rc = SQLGetData(hstmt_select, 1, SQL_C_DEFAULT, *data, pIndicator, &pIndicator););
		return MYSQLSUCCESS(rc) ? S_OK : E_FAIL;
	}
	else
		return E_FAIL;
}

//bool exportFiles(const std::vector<const std::string> & fileNames, const std::string & path);//implemented in storage
//SQLParamOptions


//start backup
///////////////////////////////////////////////////////////////////////

const char g_SQL_SELECTCORTEGE[] = "SELECT * from %s where stamp > ?";

const char g_SQL_UPSERTVALUES[] = "merge %s as target using (values(?, ?, ?))\
								  as source(name, value, stamp)\
								  on target.name = source.name when matched then\
								  update set target.stamp = source.stamp, target.value = source.value\
								  when not matched then\
								  insert values(source.name, source.value, source.stamp);";




HRESULT backupAux(HDBC fromHdbc, const string & fromPath, HDBC toHdbc, const string & toPath,
	const SQL_TIMESTAMP_STRUCT & date, UINT & amountBackup)
{
	
	//create table
	//use hstmt_upsert for creation table
	HSTMT hstmt_select, hstmt_upsert;
	SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, fromHdbc, &hstmt_select);
	check_rc(rc, E_UNEXPECTED);
	BOOST_SCOPE_EXIT(hstmt_select){ SQLFreeHandle(SQL_HANDLE_STMT, hstmt_select); } BOOST_SCOPE_EXIT_END;
	
	rc = SQLAllocHandle(SQL_HANDLE_STMT, toHdbc, &hstmt_upsert);
	check_rc(rc, E_UNEXPECTED);
	BOOST_SCOPE_EXIT(hstmt_upsert){ SQLFreeHandle(SQL_HANDLE_STMT, hstmt_upsert); } BOOST_SCOPE_EXIT_END;
	
	string sqlQuery;
	//bool retVal = false;
	bind2Params(toPath, toPath, (char *)g_SQL_CREATE, sqlQuery);
	rc = SQLExecDirect(hstmt_upsert, (SQLCHAR*)sqlQuery.c_str(), sqlQuery.size());
	check_rc(rc, E_FAIL);
	SQLFreeStmt(hstmt_upsert, SQL_CLOSE);

	//select tuples
	bindParam(fromPath, (char *)g_SQL_SELECTCORTEGE, sqlQuery);
	SQLPrepare(hstmt_select, (SQLCHAR*)sqlQuery.c_str(), SQL_NTS);
	rc = SQLBindParameter(hstmt_select, 1, SQL_PARAM_INPUT, SQL_C_TIMESTAMP, SQL_TYPE_TIMESTAMP,
		27, 7, (SQLPOINTER*)&date, sizeof(SQL_TIMESTAMP_STRUCT), NULL);
	check_rc(rc, E_UNEXPECTED);
	rc = SQLExecute(hstmt_select);
	check_rc(rc, E_FAIL);
	string copyName(g_maxColumnNameSize, '\0'), copyValue(g_maxColumnValueSize,'\0');
	SQLINTEGER copyNameSize = 0, copyValueSize = 0;
	SQL_TIMESTAMP_STRUCT copyDate;

	bindParam(toPath, (char *)g_SQL_UPSERTVALUES, sqlQuery);
	rc = SQLPrepare(hstmt_upsert, (SQLCHAR*)sqlQuery.c_str(), SQL_NTS);
	check_rc(rc, E_UNEXPECTED);

	rc = SQLBindParameter(hstmt_upsert, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
		g_maxColumnNameSize, 0, &copyName[0], g_maxColumnNameSize, &copyNameSize);
	check_rc(rc, E_UNEXPECTED);
	rc = SQLBindParameter(hstmt_upsert, 2, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY,
		/*columnSize*/g_maxColumnValueSize, 0, &copyValue[0], /*bufferSize*/g_maxColumnValueSize, &copyValueSize);
	check_rc(rc, E_UNEXPECTED);
	rc = SQLBindParameter(hstmt_upsert, 3, SQL_PARAM_INPUT, SQL_C_TIMESTAMP, SQL_TYPE_TIMESTAMP,
		27, 7, &copyDate, sizeof(SQL_TIMESTAMP_STRUCT), 0);
	check_rc(rc, E_UNEXPECTED);
	
	
	while ((rc = SQLFetch(hstmt_select)) == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
	{
		rc = SQLGetData(hstmt_select, 1, SQL_C_CHAR, &copyName[0], g_maxColumnNameSize, &copyNameSize);
		check_rc(rc, RPC_E_INVALID_DATA);
		rc = SQLGetData(hstmt_select, 2, SQL_C_BINARY, &copyValue[0], g_maxColumnValueSize, &copyValueSize);
		check_rc(rc, RPC_E_INVALID_DATA);
		rc = SQLGetData(hstmt_select, 3, SQL_C_TYPE_TIMESTAMP, &copyDate, 0, 0);
		check_rc(rc, RPC_E_INVALID_DATA);
		rc = SQLExecute(hstmt_upsert);
		if (!MYSQLSUCCESS(rc)) break;
		++amountBackup;
	}
	return rc == SQL_NO_DATA ? S_OK : E_FAIL;
}


HRESULT _CCONV MSSQLStorage::backupFull(const char * path, UINT * amountChanged)
{
	HDBC newHdbc;
	StringParser otherParser(path);
	SQLRETURN rc = connectToDb(henv, otherParser, newHdbc);
	check_rc(rc, E_FAIL);
	
	SQL_TIMESTAMP_STRUCT s =
	{ 1987, 6, 5, 12, 34, 45, 123456700 };
	UINT amountBackup = 0;
	HRESULT retVal = backupAux(hdbc, parser.getTableName(), newHdbc, otherParser.getTableName(), s, amountBackup);
	SQLDisconnect(newHdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, newHdbc);
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return retVal;
}


const char g_SQL_CREATEDATE[] = "IF OBJECT_ID('_syslastbackupinc') IS NULL create table\
								_syslastbackupinc(fromPath VARCHAR(250), toPath VARCHAR(250), stamp DATETIME2(7))";

const char g_SQL_UPSERTDATE[] = "merge _syslastbackupinc as target using (values(?, ?, ?))\
								as source(fromPath, toPath, stamp)\
								on target.fromPath = source.fromPath and target.toPath = source.toPath when matched then\
								update set target.stamp = source.stamp when not matched then\
								insert values(source.fromPath, source.toPath, source.stamp);";

const char g_SQL_GETCURDATE[] = "select CURRENT_TIMESTAMP";

const char g_SQL_SELECTLASTDATE[] = "SELECT stamp from _syslastbackupinc where fromPath = '%s' and toPath = '%s'";



bool getTimeStampFromCQuery(HSTMT hstmt, SQLCHAR * query, SQL_TIMESTAMP_STRUCT & stamp)
{
	SQLRETURN rc = SQLExecDirect(hstmt, query, SQL_NTS);
	if (!MYSQLSUCCESS(rc))
		return false;
	rc = SQLFetch(hstmt);
	if (rc == SQL_NO_DATA)
	{
		SQLFreeStmt(hstmt, SQL_CLOSE);
		return true;
	}
	rc = SQLGetData(hstmt, 1, SQL_C_TYPE_TIMESTAMP, &stamp, sizeof(SQL_TIMESTAMP_STRUCT), 0);
	SQLFreeStmt(hstmt, SQL_CLOSE);
	return MYSQLSUCCESS(rc);
}


HRESULT _CCONV MSSQLStorage::backupIncremental(const char * path, UINT * amountChanged)
{
	HDBC newHdbc;
	StringParser otherParser(path);
	SQLRETURN rc = connectToDb(henv, otherParser, newHdbc);
	check_rc(rc, E_FAIL);
	BOOST_SCOPE_EXIT(newHdbc){
		SQLDisconnect(newHdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, newHdbc);
	} BOOST_SCOPE_EXIT_END;
	HSTMT hstmt;
	rc = SQLAllocHandle(SQL_HANDLE_STMT, newHdbc, &hstmt);
	check_rc(rc, E_UNEXPECTED);
	BOOST_SCOPE_EXIT(newHdbc) { SQLFreeHandle(SQL_HANDLE_STMT, newHdbc); } BOOST_SCOPE_EXIT_END;

	rc = SQLExecDirect(hstmt, (SQLCHAR*)g_SQL_CREATEDATE, strlen(g_SQL_CREATEDATE));
	check_rc(rc, E_FAIL);
	SQLFreeStmt(hstmt, SQL_CLOSE);
	SQL_TIMESTAMP_STRUCT oldDate = { 1987, 6, 5, 12, 34, 45, 123456700 }, nowDate;
	string query;
	bind2Params(parser.getTableName(), otherParser.getTableName(), g_SQL_SELECTLASTDATE, query);
	if (!getTimeStampFromCQuery(hstmt, (SQLCHAR*)query.c_str(), oldDate) ||
		!getTimeStampFromCQuery(hstmt, (SQLCHAR*)g_SQL_GETCURDATE, nowDate))
		return RPC_E_INVALID_DATA;
	
	UINT amountBackup = 0;
	HRESULT retVal = backupAux(hdbc, parser.getTableName(), newHdbc, otherParser.getTableName(), oldDate, amountBackup);
	if (SUCCEEDED(retVal))
	{
		string name = parser.getTableName(), otherName = otherParser.getTableName();
		SQLINTEGER s1 = name.size(), s2 = otherName.size();
		rc = SQLPrepare(hstmt, (SQLCHAR*)g_SQL_UPSERTDATE, SQL_NTS);
		check_rc(rc, E_UNEXPECTED);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
			g_maxColumnNameSize, 0, (SQLPOINTER)name.c_str(), s1, &s1);
		check_rc(rc, E_UNEXPECTED);
		rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
			g_maxColumnNameSize, 0, (SQLPOINTER)otherName.c_str(), s2, &s2);
		check_rc(rc, E_UNEXPECTED);
		rc = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_TIMESTAMP, SQL_TYPE_TIMESTAMP,
			27, 7, &nowDate, sizeof(SQL_TIMESTAMP_STRUCT), 0);
		check_rc(rc, E_UNEXPECTED);
		rc = SQLExecute(hstmt);
	}
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return SUCCEEDED(retVal) && MYSQLSUCCESS(rc) ? S_OK : E_FAIL;
}

HRESULT _CCONV MSSQLStorage::remove(const char * name)
{
	if (name == nullptr)
		return E_INVALIDARG;
	SQLRETURN rc = SQLBindParameter(hstmt_remove, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, g_maxColumnNameSize, 0,
		(SQLPOINTER*)name, strlen(name), NULL);
	check_rc(rc, E_UNEXPECTED);
	rc = SQLExecute(hstmt_remove);
	SQLFreeStmt(hstmt_remove, SQL_CLOSE);
	return MYSQLSUCCESS(rc) ? S_OK : S_FALSE;
}

MSSQLStorage::~MSSQLStorage()
{

	if (hstmt_select != NULL)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt_select);
	}
	if (hstmt_upsert != NULL)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt_upsert);
	}
	if (hstmt_remove != NULL)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt_remove);
	}
	if (hdbc != NULL)
	{
		SQLDisconnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	}

	if (henv != NULL)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}