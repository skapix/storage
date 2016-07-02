#include "..\\interface_impl.h"
#include <Windows.h>
#include <Winnetwk.h>


using namespace std;


ErrorCode _CCONV SMBStorage::openStorage(const char * st)
{
	if (st == nullptr || !parser.initialize(st))
		return INVALIDARG;
	nr = new NETRESOURCE;
	
	DWORD dwResult;
	string server = parser.getServer();
	if (server.empty())
		return INVALIDARG;
	string user = parser.getLogin();
	string pwd = parser.getPass();
	LPNETRESOURCE nnr = (LPNETRESOURCE)nr;
	//((LPNETRESOURCE)nr)->dwScope = RESOURCE_GLOBALNET;
	((LPNETRESOURCE)nr)->dwType = RESOURCETYPE_DISK;
	((LPNETRESOURCE)nr)->lpLocalName = NULL;
	((LPNETRESOURCE)nr)->lpRemoteName = &server[0];
	((LPNETRESOURCE)nr)->lpProvider = NULL;
	dwResult = WNetAddConnection2((LPNETRESOURCE)nr,
		(LPSTR)pwd.empty()?NULL:pwd.c_str(),
		(LPSTR)user.empty()?NULL:user.c_str(),
		FALSE);
	if (dwResult == NO_ERROR)
		return OK;
	// TODO: take a look at msdn and make large switch
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa385413(v=vs.85).aspx
	return FAIL;
	
};

_CCONV SMBStorage::~SMBStorage()
{
	if (nr)
	{
		DWORD dwResult = WNetCancelConnection2(((LPNETRESOURCE)nr)->lpRemoteName,
			0,
			FALSE);
		delete nr;
	}
}