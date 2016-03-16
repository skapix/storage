#include "interface_impl.h"
#include "auxiliaryStorage.h"
#include <fstream>
#include <boost/version.hpp> 
#if defined(_MSC_VER) && BOOST_VERSION==105700 
//#pragma warning(disable:4003) 
#define BOOST_PP_VARIADICS 0 
#endif
#include <boost/scope_exit.hpp>


HRESULT _CCONV Storage_impl::exportFiles(const char ** fileNames, const UINT amount, const char * path)
{
	if (!createDir(path))
		return CO_E_FAILEDTOGETWINDIR;//or E_ACCESSDENIED
	
	UINT curSize = g_maxFileSize;
	char * data = (char*)CoTaskMemAlloc(curSize);
	if (data == NULL)
		return E_OUTOFMEMORY;
	//BOOST_SCOPE_EXIT_ALL c++11 only :(
	BOOST_SCOPE_EXIT(data){ if (data) CoTaskMemFree(data); } BOOST_SCOPE_EXIT_END
	bool res = true;
	for (size_t i = 0; i < amount && res; i++)
	{
		UINT dataSize = curSize;
		HRESULT comRes = get(fileNames[i], &data, &dataSize);
		if (FAILED(comRes)) return comRes;
		if (dataSize > curSize)
		{
			curSize = dataSize;
			data = (char*)CoTaskMemRealloc(data, curSize);
			if (data == NULL)
				return E_OUTOFMEMORY;
		}
		const char * name = getFileNameFromPath(fileNames[i]);
		std::string full_path = makePathFile(path, name);//can throw exceptions :(
		std::ofstream f(full_path, std::ios::binary | std::ofstream::out);
		if (!f.is_open())
			return CO_E_FAILEDTOCREATEFILE;
		f.write(data, dataSize);//shouldn't throw exception
		res &= f.good();
		f.close();
	}
	return S_OK;
}