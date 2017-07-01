#pragma once
#ifdef HAS_SMB

#include "../export_impl.h"
#include "../FS/FSStorage.h"

class SMBStorage : public FSBase
{
  void * nr; //LPNETRESOURCE
public:
  SMBStorage() : nr(nullptr) {}
  ErrorCode _CCONV openStorage(const char * name) override;
  ~SMBStorage() override;
};

#endif // HAS_SMB