#pragma once
#ifdef HAS_MSSQL

#include "../export_impl.h"

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

#endif // HAS_MSSQL