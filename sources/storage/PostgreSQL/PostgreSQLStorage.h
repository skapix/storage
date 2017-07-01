#pragma once
#ifdef HAS_POSTGRESQL

#include "../export_impl.h"

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

#endif // HAS_POSTGRESQL