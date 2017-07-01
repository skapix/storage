#pragma once
#include "Storage.h"
#include "../export_impl.h"

/** \brief Base class for FS and SMB storages
*/
class FSBase : public Storage_impl
{
public:
  FSBase() {}
  ErrorCode _CCONV add(const char * name, const char * data, const unsigned size) override;
  ErrorCode _CCONV get(const char * name, char ** data, unsigned * size) override;
  //ErrorCode _CCONV exportFiles(const char * const * fileNames, const unsigned amount, const char * path); // implemented in Storage_impl
  ErrorCode _CCONV backupFull(const char * path, unsigned * amountChanged) override;
  ErrorCode _CCONV backupIncremental(const char * path, unsigned * amountChanged) override;
  ErrorCode _CCONV remove(const char * name) override;
  ~FSBase() {};
};

class FSStorage : public FSBase
{
public:
  FSStorage() {}
  ErrorCode _CCONV openStorage(const char * name) override;
  ~FSStorage() override;
};