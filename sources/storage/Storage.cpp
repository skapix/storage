#include "storage.h"
#include "interface_impl.h"

Storage * createStorage(const Storage_t storType)
{
	switch (storType)
	{
	case e_FSStorage:
		return new FSStorage;
	case e_FTPStorage:
		return new FTPStorage;
	case e_SMBStorage:
		return new SMBStorage;
	case e_MSSQLStorage:
		return new MSSQLStorage;
	case e_PostgreSQL:
		return new PostgreSQLStorage;
	case e_SQLite3:
		return new SQLiteStorage;
	case e_MongoDB:
		return new MongoDB;
	default:
		return nullptr;
	}
}


void releaseStorage(Storage * storage)
{
	delete storage;
}