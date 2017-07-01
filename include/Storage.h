#pragma once

#include "ErrorCode.h"
#ifdef _CMAKE_USED
#include "storage_exports.h"
#endif

#ifndef STORAGE_EXPORT
#define STORAGE_EXPORT
#endif

// calling convention
#define _CCONV __stdcall


extern "C"
{
/**
\brief Main interface.
*/
struct Storage
{
	/** Initialize storage
	@param name specific for each storage. Null-terminated
	*/
	virtual ErrorCode _CCONV openStorage(const char * name) = 0;
	/** Insert file into storage
	@param name Filename. Null-terminated
	@param data Raw data of the file.
	@param size Size of raw Data
	*/
	virtual ErrorCode _CCONV add(const char * name, const char * data, const unsigned size) = 0;
	/** Retrieve data or size depending on params
	@param name Filename. Null-terminated.
	@param data File data. Can be allocated by user, by system
	@param size Buffer (data) size.\n
	size = 0 => allocates memory with CoTaskMemAlloc and stores data. The memory should be deallocated by caller.\n
	not enough size for data => set size only\n
	enough size for data => set relevant data and size\n
	*/
	virtual ErrorCode _CCONV get(const char * name, char ** data, unsigned * size) = 0;
	/** Save files in filesystem
	@param fileNames Array of null-terminated file names.
	@param amount of files to insert
	@param path System path (directory) where files will be copied.
	*/
	virtual ErrorCode _CCONV exportFiles(const char * const * fileNames, const unsigned amount, const char * path) = 0;
	/** Full database backup
	@param path the same as name in openStorage
	@param amountChanged amount of copied files
	*/
	virtual ErrorCode _CCONV backupFull(const char * path, unsigned * amountChanged) = 0;
	/** Incremental database backup
	@param path the same as name in openStorage
	@param amountChanged amount of copied or renewed files
	*/
	virtual ErrorCode _CCONV backupIncremental(const char * path, unsigned * amountChanged) = 0;
	/** Remove file from storage
	*/
	virtual ErrorCode _CCONV remove(const char * name) = 0;
};

enum Storage_t
{
	e_FSStorage,
	e_FTPStorage,
	e_SMBStorage,
	e_MSSQLStorage,
	e_PostgreSQL,
	e_SQLite3,
	e_MongoDB
};

// TODO: pass arguments for creating a storage and remove init method
// One storage is for one server and type of connection
STORAGE_EXPORT Storage * createStorage(const Storage_t storType);
STORAGE_EXPORT void releaseStorage(Storage * storage);

#undef STORAGE_EXPORT

} // extern "C"
