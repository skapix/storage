#pragma once
#include <objbase.h>

//calling convention
#define _CCONV __stdcall

extern "C"
{
	struct Storage : public IUnknown
	{
		virtual HRESULT _CCONV openStorage(IN const char * name) = 0;
		virtual HRESULT _CCONV add(IN const char * name, IN const char * data, IN const UINT size) = 0;
		virtual HRESULT _CCONV get(IN const char * name, IN OUT OPTIONAL char ** data, IN OUT UINT * size) = 0;
		virtual HRESULT _CCONV exportFiles(IN const char ** fileNames, IN const UINT amount, IN const char * path) = 0;
		virtual HRESULT _CCONV backupFull(IN const char * path, OUT OPTIONAL UINT * amountChanged) = 0;
		virtual HRESULT _CCONV backupIncremental(IN const char * path, OUT OPTIONAL UINT * amountChanged) = 0;
		//virtual HRESULT _CCONV remove(const char * name) = 0;//TODO
	};
}

//{8E1CB9E8 - 35CD - 4A16 - 9AE1 - 62D385B664A2}
DEFINE_GUID(IID_IFlatStorage, 0x8E1CB9E8, 0x35CD, 0x4A16,
	0x9A, 0xE1, 0x62, 0xD3, 0x85, 0xB6, 0x64, 0xA2);
//{EF411E75 - 124C - 48F5 - B849 - 1EEA9ACC6FD6}
DEFINE_GUID(IID_ISMBStorage, 0xEF411E75, 0x124C, 0x48F5,
	0xB8, 0x49, 0x1E, 0xEA, 0x9A, 0xCC, 0x6F, 0xD6);
//{D259321B - 5797 - 430A - 8B76 - 01FF76CE062C}
DEFINE_GUID(IID_IFTPStorage, 0xD259321B, 0x5797, 0x430A,
	0x8B, 0x76, 0x01, 0xFF, 0x76, 0xCE, 0x06, 0x2C);
//{647B7F1A - 86EA - 47CE - 90E7 - 85C8FD634556}
DEFINE_GUID(IID_IMSSQLStorage, 0x647B7F1A, 0x86EA, 0x47CE,
	0x90, 0xE7, 0x85, 0xC8, 0xFD, 0x63, 0x45, 0x56);
//{2C3F5738 - 4999 - 4B07 - A8BA - F20C2661C180}
DEFINE_GUID(IID_IPostgreSQLStorage, 0x2C3F5738, 0x4999, 0x4B07,
	0xA8, 0xBA, 0xF2, 0x0C, 0x26, 0x61, 0xC1, 0x80);
//{954FB69F - 51D5 - 4A76 - 8CA7 - 5A1697D4F122}
DEFINE_GUID(IID_ISQLiteStorage, 0x954FB69F, 0x51D5, 0x4A76,
	0x8C, 0xA7, 0x5A, 0x16, 0x97, 0xD4, 0xF1, 0x22);
//{5122683E - 6211 - 4D24 - 9999 - 574AAD914555}
DEFINE_GUID(IID_IMongoDB, 0x5122683E, 0x6211, 0x4D24,
	0x99, 0x99, 0x57, 0x4A, 0xAD, 0x91, 0x45, 0x55);

//429E1A30-E354-42AA-A43F-63AFA7ABC537
DEFINE_GUID(CLSID_ComponentStorage, 0x429E1A30, 0xE354, 0x42AA, 
	0xA4, 0x3F, 0x63, 0xAF, 0xA7, 0xAB, 0xC5, 0x37);


STDAPI DllCanUnloadNow();
STDAPI DllGetClassObject(const CLSID& clsid, const IID& iid, void** ppv);
STDAPI DllRegisterServer();
STDAPI DllUnregisterServer();