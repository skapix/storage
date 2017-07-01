#pragma once
#include "Storage.h"
#include "StringParser.h"


/** \brief Implements most common methods/members of all storages

  Implements exportFiles, which is common for storages
*/
struct Storage_impl : public Storage
{
protected:
	StringParser parser;
public:
	Storage_impl() {}
	virtual ErrorCode _CCONV exportFiles(const char * const * fileNames, const unsigned amount, const char * path) override;
	virtual ~Storage_impl() {}
};