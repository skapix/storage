#include <initguid.h>
#include "auxiliary.h"
#include "tests.h"
#include <random>
#include <chrono>
#include <fstream>

using namespace std;

bool g_soloTest = false;
UINT g_numToTest = -1;


const IID g_IIDClasses[] = { IID_IFlatStorage, IID_ISMBStorage, IID_IFTPStorage,
		IID_IMSSQLStorage, IID_IPostgreSQLStorage, IID_ISQLiteStorage, IID_IMonUSERB };

const char * g_initParams[] = { "D:/gradWork/for_testing/flat_storage/1", //Flat
"server = \\\\USER\\Common\\1", //SMB//dirs should be created!
"server = ftp://192.168.1.101/flat_storage/1", ///"server = ftp://localhost/1", //"server = ftp://localhost, login = user, pwd = pwd" //Ftp
"Server = USER, Database = mainDB, table = main, user = anon, pwd = pwd", //MsSql
"server = localhost, port = 5432, db = main, table = main, user = anon, passwd = pwd", //PostGre
"dbname = D:\\gradWork\\for_testing\\SQL1.db, table = main, user = admin", //sqlite
"server = localhost, port = 27017, database = main, collection = main" //monUSERb
};

const char * g_fileNames[] = { "testA.txt", "testB.txt", "testC.txt", "testD.txt", "testE.txt", "testF.txt" };
const UINT g_fileAmount = sizeof(g_fileNames) / sizeof(g_fileNames[0]);
vector<string> g_fileDatas(g_fileAmount);

const char * g_fileNamesIncBackup[] = { "testA.txt", "testG.txt" };
const UINT g_fileAmountIncBackup = sizeof(g_fileNamesIncBackup) / sizeof(g_fileNamesIncBackup[0]);
vector<string> g_fileDatasIncBackup(g_fileAmountIncBackup);

const char * g_FullBackupParams[] = { "D:/gradWork/for_testing/flat_storage/1fb", //Flat
"server = \\\\USER\\Common\\1fb", //SMB
"server = ftp://localhost/1fb", //Ftp
"Server = USER, Database = mainDB, table = mainfb, user = anon, pwd = pwd", //MsSql
"server = localhost, port = 5432, db = main, table = mainfb, user = anon, passwd = pwd", //postgre
"dbname = D:\\gradWork\\for_testing\\SQL1fb.db, table = main, user = admin", //sqlite
"server = localhost, port = 27017, database = main, collection = mainfb"
};

const char * g_IncBackupParams[] = { "D:/gradWork/for_testing/flat_storage/1ib", //Flat
"server = \\\\USER\\Common\\1ib", //SMB
"server = ftp://localhost/1ib", //Ftp
"Server = USER, Database = mainDB, table = mainib, user = anon, pwd = pwd", //MsSql
"server = localhost, port = 5432, db = main, table = mainib, user = anon, passwd = pwd", //postgre
"dbname = D:\\gradWork\\for_testing\\SQL1ib.db, table = main, user = admin", //sqlite
"server = localhost, port = 27017, database = main, collection = mainib"
};

//#0 is allocated, not initialized, all pointers are buffers of size g_amountClasses
void initStorages(Storage ** buf, bool * initialized, const char ** initParams)
{
	assert(buf[0] != nullptr);
	HRESULT res = S_OK;
	for (unsigned i = 0; i < g_amountClasses; i++)
	{
		if (i >= 1)
		{
			res = buf[0]->QueryInterface(g_IIDClasses[i], (void**)&buf[i]);
			if (FAILED(res) || buf[i] == nullptr)
				throw exception("Can't get component");
		}
		res = buf[i]->openStorage(initParams[i]);
		initialized[i] = SUCCEEDED(res);
		if (!initialized[i])
			cout << "Can't initialize object #" << i << endl;
	}
}

void totalInitStorages(Storage ** buf, bool * initialized, const char ** initParams)
{
	memset(buf, NULL, sizeof(buf));
	for (size_t i = 0; i < g_amountClasses; i++)
		initialized[i] = false;
	unsigned startInit = g_soloTest ? g_numToTest : 0;
	HRESULT res = CoCreateInstance(CLSID_ComponentStorage,
		NULL,
		CLSCTX_INPROC_SERVER,
		g_IIDClasses[startInit],
		(void**)&buf[startInit]);
	if (FAILED(res))
		throw exception("Module not found");
	if (g_soloTest)
	{
		initialized[startInit] = SUCCEEDED(buf[startInit]->openStorage(initParams[startInit]));
		ASSERT_TRUE(initialized[startInit]);
	}
	else
		initStorages(buf, initialized, initParams);
}


//don't forget to start services
class StorageTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		HRESULT res = CoInitialize(NULL);
		if (FAILED(res))
			throw exception("Can't initialize com library");
		totalInitStorages(stor, initialized, g_initParams);
		
		
		unsigned seedGen = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
		//cout << "Seed: " << seedGen << endl << endl;
		rnd.seed(seedGen);

		for (size_t i = 0; i < g_fileAmount; i++)
		{
			g_fileDatas[i] = getRandData(g_minFileSize, g_maxFileSize, rnd);
		}
		for (size_t i = 0; i < g_fileAmountIncBackup; i++)
		{
			g_fileDatasIncBackup[i] = getRandData(g_minFileSize, g_maxFileSize, rnd);
		}
	}
	virtual void TearDown() 
	{
		for (size_t i = 0; i < g_amountClasses; i++)
		{
			if (g_soloTest && g_numToTest != i)
				continue;
			stor[i]->Release();
		}
		CoUninitialize();
	}
	RandomGenerator rnd;
	Storage * stor[g_amountClasses];
	bool initialized[g_amountClasses];
	
};



TEST_F(StorageTest, CorrectAddGet)
{
	string dataOriginal = g_fileDatas[0];
	const char * storedFileName = g_fileNames[0];
	for (size_t i = 0; i < g_amountClasses; i++)
	{
		if (!initialized[i])
			continue;
		//#0 add file into storage
		ASSERT_EQ(stor[i]->add(storedFileName, dataOriginal.data(),dataOriginal.size()), S_OK);
		//#1 get size of file
		UINT filesize = 0;
		ASSERT_EQ(stor[i]->get(storedFileName, nullptr, &filesize), S_OK);
		ASSERT_EQ(dataOriginal.size(), filesize);
		//#2 let com object to alloc some memory
		char * filedata = nullptr;
		filesize = 0;
		ASSERT_EQ(stor[i]->get(storedFileName, &filedata, &filesize), S_OK);
		ASSERT_NE(filedata, nullptr);
		ASSERT_EQ(dataOriginal.size(), filesize);
		ASSERT_EQ(memcmp(dataOriginal.data(), filedata, filesize), 0);
		CoTaskMemFree(filedata);
		//#3.1 we allocate not enough memory
		filesize = (UINT)dataOriginal.size() - 1;
		filedata = (char*)CoTaskMemAlloc(filesize);
		ASSERT_NE(filedata, nullptr);
		ASSERT_EQ(stor[i]->get(storedFileName, &filedata, &filesize), STG_E_INSUFFICIENTMEMORY);
		ASSERT_EQ(dataOriginal.size(), filesize);
		//#3.2 we allocate enough memory
		filedata = (char*)CoTaskMemRealloc(filedata, filesize);
		ASSERT_NE(filedata, nullptr);
		ASSERT_EQ(stor[i]->get(storedFileName, &filedata, &filesize), S_OK);
		ASSERT_EQ(dataOriginal.size(), filesize);
		ASSERT_EQ(memcmp(dataOriginal.data(), filedata, filesize), 0);
		//#4 we want to get nonexistent file
		const char * unknown_filename = "this_file_shouldn't_exist_in_the_storage.err";
		filesize = (UINT)dataOriginal.size();
		ASSERT_EQ(stor[i]->get(unknown_filename, &filedata, &filesize), S_FALSE);
		CoTaskMemFree(filedata);
	}
}

TEST_F(StorageTest, CorrectExport)
{
	const string path = "D:\\gradWork\\for_testing\\auxil2";
	const size_t length_buf = sizeof(g_fileNames) / sizeof(char*);

	string recv_data;
	const vector<const string> fileNamesVec(g_fileNames, g_fileNames + length_buf);
	//check for each db
	for (size_t i = 0; i < g_amountClasses; i++)
	{
		if (!initialized[i])
			continue;
		//add elements to db
		for (size_t j = 0; j < length_buf; j++)
		{
			ASSERT_EQ(stor[i]->add(fileNamesVec[j].c_str(), g_fileDatas[j].data(), g_fileDatas[j].size()), S_OK);
		}

		ASSERT_EQ(stor[i]->exportFiles(g_fileNames, length_buf, path.c_str()), S_OK);
		//check correctness of exporting files
		for (size_t j = 0; j < length_buf; j++)
		{
			const string pathName = makePathFile(path, fileNamesVec[j]);
			ifstream f(pathName, ios::binary | ifstream::in);// ÎÁßÇÀÒÅËÜÍÎ ôëàã binary
			//make no mistake, don't forget to set binary flag!!!!!!!!
			ASSERT_TRUE(f.is_open());
			recv_data = getDataFile(f);
			f.close();
			ASSERT_EQ(g_fileDatas[j].size(), recv_data.size());
			ASSERT_EQ(g_fileDatas[j], recv_data);
		}
	}
}


TEST_F(StorageTest, CorrectBackup)
{
	Storage * bStor[g_amountClasses];
	auto checkFiles = [&](Storage * stor,const char ** filenames, const UINT fileAmount)
	{
		string buf(g_maxFileSize, 0);
		for (unsigned j = 0; j < fileAmount; j++)
		{
			UINT sizeBuf = (UINT)buf.size();
			char * pBuf = (&buf[0]);
			ASSERT_EQ(stor->get(filenames[j], &pBuf, &sizeBuf), S_OK);
		}
	};

	
	bool bInitialized[g_amountClasses];
	
	totalInitStorages(bStor, bInitialized, g_FullBackupParams);
	//full backup
	//assert all file are in the storage
	UINT sizeBuf = g_maxFileSize;
	string buf(g_maxFileSize, 0);
	UINT amountFiles;
	for (unsigned i = 0; i < g_amountClasses; i++)
	{
		if (!initialized[i] || !bInitialized[i])
			continue;
		amountFiles = 0;
		ASSERT_EQ(stor[i]->backupFull(g_FullBackupParams[i], &amountFiles), S_OK);
		ASSERT_GE(amountFiles, g_fileAmount);//6 or 7
		checkFiles(bStor[i], g_fileNames, g_fileAmount);
	}

	//incBackup
	for (unsigned i = 0; i < g_amountClasses; i++)
	{
		if (!initialized[i] || !bInitialized[i])
			continue;
		bInitialized[i] = bStor[i]->openStorage(g_IncBackupParams[i]) == S_OK;
		amountFiles = 0;
		ASSERT_EQ(stor[i]->backupIncremental(g_IncBackupParams[i],&amountFiles), S_OK);
		ASSERT_GE(amountFiles, g_fileAmount);//6 or 7
		checkFiles(bStor[i], g_fileNames, g_fileAmount);
		//adding some new files with existing names into original storage
		for (unsigned j = 0; j < g_fileAmountIncBackup; j++)
		{
			ASSERT_EQ(stor[i]->add(g_fileNamesIncBackup[j], g_fileDatasIncBackup[j].data(), (UINT)g_fileDatasIncBackup[j].size()), S_OK);
		}
		amountFiles = 0;
		ASSERT_EQ(stor[i]->backupIncremental(g_IncBackupParams[i], &amountFiles), S_OK);
		if (i==2)
			ASSERT_GE(amountFiles, g_fileAmountIncBackup);//FTP is so special
		else
			ASSERT_EQ(amountFiles, g_fileAmountIncBackup);
		checkFiles(bStor[i], g_fileNamesIncBackup, g_fileAmountIncBackup);
	}
	
	for (unsigned i = 0; i < g_amountClasses; i++)
	{
		if (g_soloTest && g_numToTest != i)
			continue;
		bStor[i]->Release();
	}
}





//TEST_F(StorageTest, CorrectRemove)
//{
//	string dataOriginal = g_fileDatas[0];
//	const char * storedFileName = "ololo_i_shouldnt_exist";
//	for (size_t i = 0; i < g_amountClasses; i++)
//	{
//		if (!initialized[i])
//			continue;
//		ASSERT_EQ(stor[i]->add(storedFileName, dataOriginal.data(), dataOriginal.size() / 1024), S_OK);
//		UINT filesize = 0;
//		ASSERT_EQ(stor[i]->get(storedFileName, nullptr, &filesize), S_OK);
//		ASSERT_EQ(stor[i]->remove(storedFileName), S_OK);
//		ASSERT_EQ(stor[i]->get(storedFileName, nullptr, &filesize), S_FALSE);
//		ASSERT_EQ(stor[i]->remove(storedFileName), S_FALSE);
//	}
//}