#pragma once
#ifdef HAS_MONGODB

#include "../export_impl.h"

class MongoDB : public Storage_impl
{
  void *client; //mongoc_client_t *
  void *collection; //mongoc_collection_t *
  void *fields; //bson_t *
public:
  MongoDB() : client(nullptr), collection(nullptr), fields(nullptr) {}
  ErrorCode _CCONV openStorage(const char * name) override;
  ErrorCode _CCONV add(const char * name, const char * data, const unsigned size) override;
  ErrorCode _CCONV get(const char * name, char ** data, unsigned * size) override;
  //ErrorCode _CCONV exportFiles(const char * const * fileNames, const unsigned amount, const char * path);//implemented in storage
  ErrorCode _CCONV backupFull(const char * path, unsigned * amountChanged) override;
  ErrorCode _CCONV backupIncremental(const char * path, unsigned * amountChanged) override;
  ErrorCode _CCONV remove(const char * name);
  ~MongoDB() override;
};

#endif // HAS_MONGODB