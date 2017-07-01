#pragma once
#ifdef HAS_FTP

#include "../export_impl.h"

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

#endif // HAS_FTP