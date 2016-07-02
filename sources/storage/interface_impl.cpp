#include "interface_impl.h"
#include "auxiliaryStorage.h"
#include "utilities/dyn_mem_manager.h"
#include "utilities/common.h"
#include <fstream>
#include <memory>
#include <boost/version.hpp> 
#include <boost/scope_exit.hpp>

using utilities::MemManager;
using utilities::makePathFile;
using std::string;

ErrorCode _CCONV Storage_impl::exportFiles(const char * const * fileNames, const unsigned amount, const char * path)
{
	if (!createDir(path))
		return FAILEDTOGETWINDIR;//or E_ACCESSDENIED

	unsigned curSize = g_maxFileSize;


	auto data = MemManager<>::create(curSize);
	if (data == nullptr)
		return OUTOFMEMORY;

	bool res = true;
	for (size_t i = 0; i < amount && res; i++)
	{
		unsigned dataSize = curSize;
		char * buf = data.get();
		ErrorCode comRes = get(fileNames[i], &buf, &dataSize);
		assert(buf == data.get());

		if (failed(comRes))
			return comRes;

		if (dataSize > curSize)
		{
			std::vector<int> t;
			curSize = dataSize;
			data.realloc(curSize);
			if (data == nullptr)
				return OUTOFMEMORY;
		}
		const char * name = getFileNameFromPath(fileNames[i]);
		std::string full_path = makePathFile(path, name); //can throw exceptions :(
		std::ofstream f(full_path, std::ios::binary | std::ofstream::out);
		if (!f.is_open())
			return FAILEDTOCREATEFILE;
		f.write(data.get(), dataSize);//shouldn't throw exception
		res &= f.good();
		f.close();
	}
	return OK;
}
