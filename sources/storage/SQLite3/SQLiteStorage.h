#pragma once
#ifdef HAS_SQLITE3

#include "../export_impl.h"

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

#endif // HAS_SQLITE3