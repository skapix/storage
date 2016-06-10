#include <initguid.h>
#include "auxiliary.h"
#include "tests.h"
#include <random>
#include <chrono>
#include <fstream>

using namespace std;

vector<map<string, string>> g_inits;

const IID g_IIDClasses[] = { IID_IFSStorage, IID_ISMBStorage, IID_IFTPStorage,
		IID_IMSSQLStorage, IID_IPostgreSQLStorage, IID_ISQLiteStorage, IID_IMongoDB };


const char * g_fileNames[] = { "testA.txt", "testB.txt", "testC.txt", "testD.txt", "testE.txt", "testF.txt" };
const UINT g_fileAmount = sizeof(g_fileNames) / sizeof(g_fileNames[0]);
vector<string> g_fileDatas(g_fileAmount);

const char * g_fileNamesIncBackup[] = { "testA.txt", "testG.txt" };
const UINT g_fileAmountIncBackup = sizeof(g_fileNamesIncBackup) / sizeof(g_fileNamesIncBackup[0]);
vector<string> g_fileDatasIncBackup(g_fileAmountIncBackup);


enum InitType
{
	original_init,
	full_backup,
	incremental_backup
};


std::string get_init_param(const unsigned i, const InitType & init_type)
{
	assert(g_inits.size() > i);
	switch (init_type)
	{
	case original_init:
		return g_inits[i][im::g_init];
	case full_backup:
		return g_inits[i][im::g_fb];
	case incremental_backup:
		return g_inits[i][im::g_ib];
	default:
		throw std::exception("bad param");
	}
}

// buf[0] is allocated, not initialized
void initStorages(vector<Storage*> & buf, vector<bool> & initialized, const InitType & init_type)
{
	assert(buf[0] != nullptr);
	assert(buf.size() == g_inits.size());
	assert(buf.size() == initialized.size());
	
	HRESULT res = S_OK;
	for (unsigned i = 0; i < buf.size(); ++i)
	{
		int n_init = atoi(g_inits.front().at(im::g_id).c_str());
		if (i >= 1)
		{
			res = buf[0]->QueryInterface(g_IIDClasses[n_init], (void**)&buf[i]);
			if (FAILED(res) || buf[i] == nullptr)
				throw exception("Can't get component");
		}
		string param = get_init_param(i, init_type);
		res = buf[i]->openStorage(param.c_str());
		initialized[i] = SUCCEEDED(res);
		if (!param.empty() && !initialized[i])
			cout << "Can't initialize object #" << i << endl;
	}
}

void totalInitStorages(vector<Storage*> & buf, vector<bool> & initialized, const InitType & init_type)
{
	if (g_inits.empty())
	{
		buf.clear();
		initialized.clear();
		return;
	}
	buf.resize(g_inits.size(), nullptr);
	initialized.resize(g_inits.size(), false);
	int start_init = atoi(g_inits.front().at(im::g_id).c_str());
	HRESULT res = CoCreateInstance(CLSID_ComponentStorage,
		NULL,
		CLSCTX_INPROC_SERVER,
		g_IIDClasses[start_init],
		(void**)&buf[0]);
	if (FAILED(res))
		throw exception("Module not found");

	initStorages(buf, initialized, InitType::original_init);
}


class StorageTest : public ::testing::Test {
private:
protected:
	virtual void SetUp() {
		HRESULT res = CoInitialize(NULL);
		if (FAILED(res))
			throw exception("Can't initialize com library");
		totalInitStorages(stor, initialized, InitType::original_init);
		
		
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
		for (size_t i = 0; i < g_inits.size(); ++i)
		{
			stor[i]->Release();
		}
		CoUninitialize();
	}
	RandomGenerator rnd;
	std::vector<Storage *> stor;
	std::vector<bool> initialized;
	
};



TEST_F(StorageTest, CorrectAddGet)
{
	string dataOriginal = g_fileDatas[0];
	const char * storedFileName = g_fileNames[0];
	for (size_t i = 0; i < g_inits.size(); ++i)
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
	const vector<string> fileNamesVec(g_fileNames, g_fileNames + length_buf);
	//check for each db
	for (size_t i = 0; i < g_inits.size(); ++i)
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
	std::vector<Storage*> bStor;
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

	
	std::vector<bool> bInitialized;
	totalInitStorages(bStor, bInitialized, InitType::full_backup);
	//full backup
	//assert all file are in the storage
	UINT sizeBuf = g_maxFileSize;
	string buf(g_maxFileSize, 0);
	UINT amountFiles;
	for (unsigned i = 0; i < g_inits.size(); ++i)
	{
		if (!initialized[i] || !bInitialized[i])
			continue;
		amountFiles = 0;
		string param = get_init_param(i, InitType::full_backup);
		ASSERT_EQ(stor[i]->backupFull(param.c_str(), &amountFiles), S_OK);
		ASSERT_GE(amountFiles, g_fileAmount);//6 or 7
		checkFiles(bStor[i], g_fileNames, g_fileAmount);
	}

	//incBackup
	for (unsigned i = 0; i < g_inits.size(); ++i)
	{
		if (!initialized[i] || !bInitialized[i])
			continue;
		string param = get_init_param(i, InitType::incremental_backup);
		bInitialized[i] = bStor[i]->openStorage(param.c_str()) == S_OK;
		amountFiles = 0;
		ASSERT_EQ(stor[i]->backupIncremental(param.c_str(), &amountFiles), S_OK);
		ASSERT_GE(amountFiles, g_fileAmount); // 6 or 7
		checkFiles(bStor[i], g_fileNames, g_fileAmount);
		//adding some new files with existing names into original storage
		for (unsigned j = 0; j < g_fileAmountIncBackup; j++)
		{
			ASSERT_EQ(stor[i]->add(g_fileNamesIncBackup[j], g_fileDatasIncBackup[j].data(), (UINT)g_fileDatasIncBackup[j].size()), S_OK);
		}
		amountFiles = 0;
		ASSERT_EQ(stor[i]->backupIncremental(param.c_str(), &amountFiles), S_OK);
		if (i==2)
			ASSERT_GE(amountFiles, g_fileAmountIncBackup);//FTP is so special
		else
			ASSERT_EQ(amountFiles, g_fileAmountIncBackup);
		checkFiles(bStor[i], g_fileNamesIncBackup, g_fileAmountIncBackup);
	}
	
	for (unsigned i = 0; i < g_inits.size(); ++i)
	{
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