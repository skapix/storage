#pragma warning(push)
#pragma warning(disable:4996)

#include "../auxiliaryStorage.h" //getFunc
#include "../interface_impl.h"
#include "bcon.h"
#include "mongoc.h"
#include <sstream>
#include "boost/scope_exit.hpp"
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>

using std::string;
using namespace boost::archive::iterators;

const char g_name[] = "name", g_value[] = "value", g_id[] = "_id";


bool setClientCollection(const StringParser & parser, mongoc_client_t *& client, mongoc_collection_t *& collection)
{
	string clientPath = "mongodb://";
	clientPath.append(parser.getLogin().empty() ? parser.getServer() + ":" + parser.getPort() :
		parser.getLogin() + ":" + parser.getPass() + "@" + parser.getServer() + ":" + parser.getPort());
	client = mongoc_client_new(clientPath.c_str());
	if (client == NULL)
		return false;
	collection = mongoc_client_get_collection(client, parser.getDbName().c_str(), parser.getTableName().c_str());
	if (collection == NULL)
	{
		mongoc_client_destroy(client);
		return false;
	}
	return true;
}

inline bool createUniqueIndexName(mongoc_collection_t * collection)
{
	mongoc_index_opt_t opt;
	mongoc_index_opt_init(&opt);
	opt.name = g_name;
	opt.unique = true;
	opt.is_initialized = true;
	bson_t * doc = bson_new();
	if (!doc)
		return false;
	bool bval = bson_append_int32(doc, g_name, strlen(g_name), 1);
	if (!bval)
	{
		bson_destroy(doc);
		return false;
	}
	bval = mongoc_collection_create_index(collection, doc, &opt, NULL);
	if (!bval)
	{
		bson_destroy(doc);
		return false;
	}
	bson_destroy(doc);
	return true;
}

ErrorCode _CCONV MongoDB::openStorage(const char * dataPath)
{
	if (dataPath == nullptr || !parser.initialize(dataPath))
		return INVALIDARG;
	mongoc_init();
	if (!setClientCollection(parser, (mongoc_client_t *&)client, (mongoc_collection_t*&)collection))
		return FAIL;
	if (!createUniqueIndexName((mongoc_collection_t*)collection))
		return FAIL;

	fields = bson_new();
	bson_append_int32((bson_t*)fields, g_id, strlen(g_id), 0);
	bson_append_int32((bson_t*)fields, g_value, strlen(g_value), 1);
	return OK;
}



inline void toBase64(const char * begin, const char * end, string & st)
{
	std::ostringstream os;
	typedef
		base64_from_binary<
		transform_width< const char *, 6, 8>
		>
		base64_text;

	std::copy(
		base64_text(begin),
		base64_text(end),
		ostream_iterator<char>(os));

	st.assign(os.str());
}

ErrorCode _CCONV MongoDB::add(const char * name, const char * data, const unsigned size)
{
	if (name == nullptr || data == nullptr)
		return INVALIDARG;
	string dataBase64;
	toBase64(data, data + size, dataBase64);
	bson_t * query = bson_new(), * update = bson_new();
	BOOST_SCOPE_EXIT(query, update) {
		bson_destroy(query); 
		bson_destroy(update);
	} BOOST_SCOPE_EXIT_END;
	//bson_oid_t oid;
	//bson_context_t * context = bson_context_new(BSON_CONTEXT_NONE);//bson_context_get_default()
	//bson_oid_init(&oid, context);
	//bson_context_destroy(context);

	if (!bson_append_utf8(query, g_name, strlen(g_name), name, strlen(name)) ||
		//!bson_append_oid(update, g_id, strlen(g_id), &oid) ||
		!bson_append_utf8(update, g_name, strlen(g_name), name, strlen(name)) ||
		!bson_append_utf8(update, g_value, strlen(g_value), dataBase64.data(), dataBase64.size()))
		return FAIL;
	
	if (mongoc_collection_remove((mongoc_collection_t*)collection, MONGOC_REMOVE_SINGLE_REMOVE, query, NULL, NULL) &&
		mongoc_collection_insert((mongoc_collection_t*)collection, MONGOC_INSERT_NONE, update, NULL, NULL))
		return OK;
	//bson_error_t err;
	//retVal = mongoc_collection_update((mongoc_collection_t*)collection, MONGOC_UPDATE_UPSERT, query, update, NULL, &err);
	//or
	//retVal = mongoc_collection_find_and_modify((mongoc_collection_t*)collection,
	//			query, NULL, update, NULL, false, true, false, NULL, &err);
	//if (!retVal)
	//	printf("%s", err.message);
	return FAIL;
}

bool retrieveValueFromKey(const bson_t * doc, const char * key, bson_value_t & value)
{
	bson_iter_t iter;
	if (bson_iter_init(&iter, doc))
		while (bson_iter_next(&iter))
		{
			if (strcmp(bson_iter_key(&iter), key) == 0)
			{
				value = *bson_iter_value(&iter);
				return true;
			}
		}
	return false;
}


inline void fromBase64(const char * begin, const char * end, string & st)
{
	std::ostringstream os;
	typedef transform_width<
		binary_from_base64 <const char *>, 8, 6
		> text;
	
	std::copy(
		text(begin),
		text(end),
		ostream_iterator<char>(os));
	st.assign(os.str());
}

inline void fromBase64(const char * begin, const char * end, char * st)
{
	std::ostringstream os;
	typedef transform_width<
		binary_from_base64 <const char *>, 8, 6
	> text;

	std::copy(
		text(begin),
		text(end),
		ostream_iterator<char>(os));
	memcpy(st, os.str().data(), os.str().length());
}

unsigned getLengthFromBase64(const char * begin, const unsigned length)
{
	unsigned res = length / 4 * 3;
	unsigned sum = length % 2 ? 2 : length % 4 ? 1 : 0;
	return res + sum;
}

ErrorCode _CCONV MongoDB::get(const char * name, char ** data, unsigned * size)
{
	if (name == nullptr || size == nullptr)
		return INVALIDARG;
	bson_t * query = bson_new();
	BOOST_SCOPE_EXIT(query) { bson_destroy(query); } BOOST_SCOPE_EXIT_END
	const bson_t * doc;
	bson_append_utf8(query, g_name, 4, name, strlen(name));

	mongoc_cursor_t *cursor = mongoc_collection_find((mongoc_collection_t*)collection,
		MONGOC_QUERY_NONE, 0, 0, 0, query, (bson_t*)fields, NULL);
	BOOST_SCOPE_EXIT(cursor) { mongoc_cursor_destroy(cursor); } BOOST_SCOPE_EXIT_END;
	bson_value_t val;
	if (mongoc_cursor_next(cursor, &doc) && retrieveValueFromKey(doc, g_value, val))
	{
		char * stBegin = val.value.v_utf8.str;
		unsigned len = val.value.v_utf8.len;//length of data in base64
		unsigned length = getLengthFromBase64(stBegin, len);//length of binary data
		d_getFunc(data, size, length, fromBase64(stBegin, stBegin + len, *data));
		return OK;
	}
	return EC_FALSE;
}


ErrorCode backupAux(mongoc_collection_t * collection, mongoc_collection_t * otherCollection,
	const int64_t lastBackupTime, unsigned & amountBackup)
{
	if (!createUniqueIndexName(otherCollection))
		return UNEXPECTED;
	const bson_t *doc;
	bson_t * queryFind = bson_new();
	BOOST_SCOPE_EXIT(queryFind) { bson_destroy(queryFind); } BOOST_SCOPE_EXIT_END;
	mongoc_cursor_t *cursor = mongoc_collection_find(collection,
		MONGOC_QUERY_NONE, 0, 0, 0, queryFind, NULL, NULL);
	if (!cursor)
		return UNEXPECTED;
	BOOST_SCOPE_EXIT(cursor) { mongoc_cursor_destroy(cursor); } BOOST_SCOPE_EXIT_END;
	
	while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &doc))
	{
		bson_value_t value;
		if (lastBackupTime != INT64_MIN)
		{
			if (!retrieveValueFromKey(doc, g_id, value))
				return FAIL;
			time_t modificationTime = bson_oid_get_time_t(&value.value.v_oid);
			if (modificationTime < lastBackupTime)
				continue;
		}
		if (!retrieveValueFromKey(doc, g_name, value))
			return FAIL;
		//insert
		bson_t * selector = bson_new();
		bson_append_utf8(selector, g_name, strlen(g_name), value.value.v_utf8.str, value.value.v_utf8.len);
		if (!mongoc_collection_remove(otherCollection, MONGOC_REMOVE_SINGLE_REMOVE, selector, NULL, NULL) ||
			!mongoc_collection_insert(otherCollection, MONGOC_INSERT_NONE, doc, NULL, NULL))
		{
			bson_destroy(selector);
			return FAIL;
		}
		bson_destroy(selector);
		++amountBackup;
	}
	bool r = !mongoc_cursor_error(cursor, NULL);
	
	return r ? OK : FAIL;
}


//ErrorCode exportFiles(const std::vector<const std::string> & fileNames, const std::string & path);//implemented in storage
ErrorCode _CCONV MongoDB::backupFull(const char * path, unsigned * amountChanged)
{
	if (path == nullptr)
		return INVALIDARG;
	StringParser otherParser(path);
	mongoc_client_t * otherClient;
	mongoc_collection_t * otherCollection;
	if (!setClientCollection(otherParser, otherClient, otherCollection))
		return FAIL;
	string date;
	unsigned amountBackup = 0;
	ErrorCode retVal = backupAux((mongoc_collection_t*)collection, otherCollection, INT64_MIN, amountBackup);
	mongoc_collection_destroy(otherCollection);
	mongoc_client_destroy(otherClient);
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return retVal;
}

///////////////////incremental

const char g_syslastbackupinc[] = "syslastbackupinc";
const char g_keyFrom[] = "from";
const char g_keyTo[] = "to";
const char g_time[] = "time";



int64_t retrieveLastTime(mongoc_client_t * client, const string & dbname, const string & from, const string & to)
{
	int64_t lastStamp = INT64_MIN;
	bson_t * query = bson_new(), * fieldTime = bson_new();
	BOOST_SCOPE_EXIT(query, fieldTime) { bson_destroy(query); bson_destroy(fieldTime); } BOOST_SCOPE_EXIT_END;
	const bson_t * doc;
	bson_append_utf8(query, g_keyFrom, strlen(g_keyFrom), from.c_str(), from.size());
	bson_append_utf8(query, g_keyTo, strlen(g_keyTo), to.c_str(), to.size());
	bson_append_int32(fieldTime, g_id, strlen(g_id), 0);
	bson_append_int32(fieldTime, g_time, strlen(g_time), 1);
	mongoc_collection_t * collection = mongoc_client_get_collection(client, dbname.c_str(), g_syslastbackupinc);
	BOOST_SCOPE_EXIT(collection) { mongoc_collection_destroy(collection); } BOOST_SCOPE_EXIT_END;
	mongoc_cursor_t *cursor = mongoc_collection_find(collection,
		MONGOC_QUERY_NONE, 0, 0, 0, query, fieldTime, NULL);
	BOOST_SCOPE_EXIT(cursor) { mongoc_cursor_destroy(cursor); } BOOST_SCOPE_EXIT_END;
	if (!cursor || !mongoc_cursor_next(cursor, &doc))
		return lastStamp;
	else
	{
		bson_value_t val;
		if (!retrieveValueFromKey(doc, g_time, val))
			return lastStamp;
		lastStamp = val.value.v_datetime;
	}
	return lastStamp;
}

ErrorCode upsertTime(mongoc_client_t * client, const string & dbname, const string & from, const string & to, const int64_t lastTime)
{
	mongoc_collection_t * collection = mongoc_client_get_collection(client, dbname.c_str(), g_syslastbackupinc);
	bool retVal = false;
	string dataBase64;
	bson_t * query = bson_new(), *update = bson_new();
	BOOST_SCOPE_EXIT(query, update) { bson_destroy(query); bson_destroy(update); } BOOST_SCOPE_EXIT_END;
	if (!bson_append_utf8(query, g_keyFrom, strlen(g_keyFrom), from.c_str(), from.size()) ||
		!bson_append_utf8(query, g_keyTo, strlen(g_keyTo), to.c_str(), to.size()) ||
		!bson_append_utf8(update, g_keyFrom, strlen(g_keyFrom), from.c_str(), from.size()) ||
		!bson_append_utf8(update, g_keyTo, strlen(g_keyTo), to.c_str(), to.size()) ||
		!bson_append_date_time(update, g_time, strlen(g_time), lastTime))
		return retVal ? OK : FAIL;
	retVal = mongoc_collection_find_and_modify(collection, query, NULL, update, NULL, false, true, false, NULL, NULL);
	return retVal ? OK : FAIL;
}

ErrorCode _CCONV MongoDB::backupIncremental(const char * path, unsigned * amountChanged)
{
	if (path == nullptr)
		return INVALIDARG;
	ErrorCode retVal = FAIL;
	StringParser otherParser(path);
	mongoc_client_t * otherClient;
	mongoc_collection_t * otherCollection;
	if (!setClientCollection(otherParser, otherClient, otherCollection))
		return INVALIDARG;
	string date;
	
	int64_t lastStamp = retrieveLastTime(otherClient, otherParser.getDbName(), parser.getTableName(), otherParser.getTableName());
	time_t nowStamp;
	time(&nowStamp);
	unsigned amountBackup = 0;
	retVal = backupAux((mongoc_collection_t*)collection, otherCollection, lastStamp, amountBackup);
	if (succeeded(retVal))
		retVal = upsertTime(otherClient, otherParser.getDbName(), parser.getTableName(), otherParser.getTableName(), nowStamp);
	mongoc_collection_destroy(otherCollection);
	mongoc_client_destroy(otherClient);
	if (amountChanged != nullptr)
		*amountChanged = amountBackup;
	return retVal;
}

ErrorCode _CCONV MongoDB::remove(const char * name)
{
	bson_t * selector = bson_new();
	BOOST_SCOPE_EXIT(selector) {
		bson_destroy(selector);
	} BOOST_SCOPE_EXIT_END;
	
	if (!bson_append_utf8(selector, g_name, strlen(g_name), name, strlen(name)))
		return FAIL;
	bool r = mongoc_collection_remove((mongoc_collection_t*)collection, MONGOC_REMOVE_SINGLE_REMOVE, selector, nullptr, nullptr);
	return r ? OK : EC_FALSE;
}

MongoDB::~MongoDB()
{
	if (fields)
		bson_destroy((bson_t*)fields);
	if (collection)
		mongoc_collection_destroy((mongoc_collection_t*)collection);
	if (client)
		mongoc_client_destroy((mongoc_client_t*)client);
	mongoc_cleanup();
}

#pragma warning(pop)