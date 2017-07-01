#include "storage.h"
#include "FS/FSStorage.h"
#include "SMB/SMBStorage.h"
#include "FTP/FTPStorage.h"
#include "MSSQL/MSSQLStorage.h"
#include "PostgreSQL/PostgreSQLStorage.h"
#include "SQLite3/SQLiteStorage.h"
#include "MongoDB/MongoDBStorage.h"

Storage * createStorage(const Storage_t storType)
{
	switch (storType)
	{
	case e_FSStorage:
		return new FSStorage;
#ifdef HAS_FTP
	case e_FTPStorage:
		return new FTPStorage;
#endif // HAS_FTP
#ifdef HAS_SMB
	case e_SMBStorage:
		return new SMBStorage;
#endif // HAS_SMB
#ifdef HAS_MSSQL
	case e_MSSQLStorage:
		return new MSSQLStorage;
#endif // HAS_MSSQL
#ifdef HAS_POSTGRESQL
	case e_PostgreSQL:
		return new PostgreSQLStorage;
#endif // HAS_POSTGRESQL
#ifdef HAS_SQLITE3
	case e_SQLite3:
		return new SQLiteStorage;
#endif // HAS_SQLITE3
#ifdef HAS_MONGODB
	case e_MongoDB:
		return new MongoDB;
#endif // HAS_MONGODB
	default:
		return nullptr;
	}
}


void releaseStorage(Storage * storage)
{
	delete storage;
}