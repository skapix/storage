#include <initguid.h>
#include "interface_impl.h"


static HMODULE g_hModule = NULL; // Описатель модуля DLL
static long g_cComponents = 0; // Количество активных компонентов
static long g_cServerLocks = 0; // Счетчик блокировок
// Дружественное имя компонента
const char g_szFriendlyName[] = "File storage";
// Не зависящий от версии ProgID
const char g_szVerIndProgID[] = "File.Storage";
// ProgID
const char g_szProgID[] = "File.Storage.1";

///////////////////////////////////////////////////////////
class CommonStorage : public FlatStorage, public SMBStorage, public FTPStorage,
	public MSSQLStorage, public PostgreSQLStorage, public SQLiteStorage, public MongoDB
{
public:
	// IUnknown
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv);
	virtual ULONG __stdcall AddRef();
	virtual ULONG __stdcall Release();
	CommonStorage();
	~CommonStorage();
private:
//	// Счетчик ссылок
	long m_cRef;
};

CommonStorage::CommonStorage() : m_cRef(1)
{
	InterlockedIncrement(&g_cComponents);
}

CommonStorage::~CommonStorage()
{
	InterlockedDecrement(&g_cComponents);
}
//
// Реализация IUnknown
//
HRESULT __stdcall CommonStorage::QueryInterface(const IID& iid, void** ppv)
{
	if (iid == IID_IUnknown || iid == IID_IFlatStorage)
	{
		*ppv = static_cast<FlatStorage*>(this);
	}
	else if (iid == IID_ISMBStorage)
	{
		*ppv = static_cast<SMBStorage*>(this);
	}
	else if (iid == IID_IFTPStorage)
	{
		*ppv = static_cast<FTPStorage*>(this);
	}
	else if (iid == IID_IMSSQLStorage)
	{
		*ppv = static_cast<MSSQLStorage*>(this);
	}
	else if (iid == IID_IPostgreSQLStorage)
	{
		*ppv = static_cast<PostgreSQLStorage*>(this);
	}
	else if (iid == IID_ISQLiteStorage)
	{
		*ppv = static_cast<SQLiteStorage*>(this);
	}
	else if (iid == IID_IMongoDB)
	{
		*ppv = static_cast<MongoDB*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}

ULONG __stdcall CommonStorage::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}
ULONG __stdcall CommonStorage::Release()
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}
	return m_cRef;
}

///////////////////////////////////////////////////////////
//
// Фабрика класса
//
class CFactory : public IClassFactory
{
public:
	// IUnknown
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv);
	virtual ULONG __stdcall AddRef();
	virtual ULONG __stdcall Release();
	// Интерфейс IClassFactory
	virtual HRESULT __stdcall CreateInstance(IUnknown* pUnknownOuter,
		const IID& iid,
		void** ppv);
	virtual HRESULT __stdcall LockServer(BOOL bLock);
	// Конструктор
	CFactory() : m_cRef(1) {}
	// Деструктор
	~CFactory() { /*trace("Фабрика класса:\t\tСаморазрушение");*/ }
private:
	long m_cRef;
};
//
// Реализация IUnknown для фабрики класса
//
HRESULT __stdcall CFactory::QueryInterface(const IID& iid, void** ppv)
{
	if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
	{
		*ppv = static_cast<IClassFactory*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}
ULONG __stdcall CFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}
ULONG __stdcall CFactory::Release()
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}
	return m_cRef;
}
//
// Реализация IClassFactory
//
HRESULT __stdcall CFactory::CreateInstance(IUnknown* pUnknownOuter,
	const IID& iid,
	void** ppv)
{
	//trace("Фабрика класса:\t\tСоздать компонент");
	// Агрегирование не поддерживается
	if (pUnknownOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION;
	}
	// Создать компонент
	CommonStorage * pA = new CommonStorage;
	if (pA == NULL)
	{
		return E_OUTOFMEMORY;
	}
	// Вернуть запрошенный интерфейс
	HRESULT hr = pA->QueryInterface(iid, ppv);
	// Освободить указатель на IUnknown
	pA->Release();
	return hr;
}
// LockServer
HRESULT __stdcall CFactory::LockServer(BOOL bLock)
{
	if (bLock)
	{
		InterlockedIncrement(&g_cServerLocks);
	}
	else
	{
		InterlockedDecrement(&g_cServerLocks);
	}
	return S_OK;
}

//export functions
HRESULT _CCONV DllCanUnloadNow()
{
	if ((g_cComponents == 0) && (g_cServerLocks == 0))
		return S_OK;
	else
		return S_FALSE;
}

STDAPI DllGetClassObject(const CLSID & clsid, const IID & iid, void ** ppv)
{
	// Можно ли создать такой компонент?
	if (clsid != CLSID_ComponentStorage)
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}
	// Создать фабрику класса
	CFactory* pFactory = new CFactory; // Счетчик ссылок устанавливается
	// в конструкторе в 1
	if (pFactory == NULL)
	{
		return E_OUTOFMEMORY;
	}
	// Получить требуемый интерфейс
	HRESULT hr = pFactory->QueryInterface(iid, ppv);
	pFactory->Release();
	return hr;
}


STDAPI DllRegisterServer()
{
	return RegisterServer(g_hModule,
		CLSID_ComponentStorage,
		g_szFriendlyName,
		g_szVerIndProgID,
		g_szProgID);
}

STDAPI DllUnregisterServer()
{
	return UnregisterServer(CLSID_ComponentStorage,
		g_szVerIndProgID,
		g_szProgID);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, void* lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		g_hModule = (HMODULE)hModule;
	}
	return TRUE;
}