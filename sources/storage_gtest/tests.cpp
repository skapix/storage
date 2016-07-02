#include "tests.h"
#include "auxiliary.h"
#include "utilities/common.h"
#include "utilities/dyn_mem_manager.h"

#include <random>
#include <chrono>
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>


using namespace std;

vector<pair<const string, string>> g_fileNameData =
{ make_pair("testA.txt", ""), make_pair("testB.txt", ""), make_pair("testC.txt", ""),
make_pair("testD.txt", ""), make_pair("testE.txt",""), make_pair("testF.txt","") };

vector<pair<const string, string>> g_fileNameDataIncBackup =
{ make_pair("testA.txt", ""), make_pair("testG.txt", "") };


vector<InitParam> g_testSimpleParams;
vector<ParamExtra> g_testExportParams;
vector<ParamExtra> g_testFullBackupParams;
vector<ParamExtra> g_testIncBackupParams;

using FuncType = void(*)(Storage *);

unique_ptr<Storage, FuncType> getStorage(const Storage_t type, const string & init)
{
	Storage * stor = createStorage(type);
	ErrorCode ec = stor->openStorage(init.c_str());
	if (failed(ec))
	{
		releaseStorage(stor);
		stor = nullptr;
	}
	return unique_ptr<Storage, FuncType>(stor, releaseStorage);
}

void checkFile(Storage * stor, const pair<string, string>& fileData)
{
	string buf(g_maxFileSize, 0);
	unsigned sizeBuf = (unsigned)buf.size();
	char * pBuf = &buf[0];
	ASSERT_EQ(stor->get(fileData.first.c_str(), &pBuf, &sizeBuf), OK);
	buf.resize(sizeBuf);
	ASSERT_EQ(buf, fileData.second);
}

void checkFiles(Storage * stor, const vector<pair<const string, string>> & filenames)
{
	string buf(g_maxFileSize, 0);
	for (unsigned j = 0; j < filenames.size(); ++j)
	{
		checkFile(stor, filenames[j]);
	}
}


using TestInitParam = testing::TestWithParam<InitParam>;

INSTANTIATE_TEST_CASE_P(AddGetCases, TestInitParam,
	testing::ValuesIn(g_testSimpleParams));

using TestParamExtra = testing::TestWithParam<ParamExtra>;

// TODO: add empty file correct add/get test checking
TEST_P(TestInitParam, CorrectAddGet)
{
	InitParam param = GetParam();
	auto storage = getStorage(param.type, param.initParams);
	ASSERT_TRUE(storage);

	const char * storedFileName = g_fileNameData[0].first.c_str();
	string dataOriginal = g_fileNameData[0].second;

	//#0 add file into storage
	ASSERT_EQ(storage->add(storedFileName, dataOriginal.data(), dataOriginal.size()), OK);
	//#1 get size of file
	unsigned filesize = 0;
	ASSERT_EQ(storage->get(storedFileName, nullptr, &filesize), OK);
	ASSERT_EQ(dataOriginal.size(), filesize);
	//#2 let com object to alloc some memory
	char * filedata = nullptr;
	filesize = 0;
	ASSERT_EQ(storage->get(storedFileName, &filedata, &filesize), OK);
	ASSERT_NE(filedata, nullptr);
	ASSERT_EQ(dataOriginal.size(), filesize);
	ASSERT_EQ(memcmp(dataOriginal.data(), filedata, filesize), 0);
	utilities::deallocate(filedata);
	//#3.1 we allocate not enough memory
	ASSERT_FALSE(dataOriginal.empty());
	filesize = (unsigned)dataOriginal.size() - 1;
	filedata = (char*)utilities::allocate(filesize);
	ASSERT_NE(filedata, nullptr);
	ASSERT_EQ(storage->get(storedFileName, &filedata, &filesize), INSUFFICIENTMEMORY);
	ASSERT_EQ(dataOriginal.size(), filesize);
	//#3.2 we allocate enough memory
	filedata = (char*)utilities::reallocate(filedata, filesize);
	ASSERT_NE(filedata, nullptr);
	ASSERT_EQ(storage->get(storedFileName, &filedata, &filesize), OK);
	ASSERT_EQ(dataOriginal.size(), filesize);
	ASSERT_EQ(memcmp(dataOriginal.data(), filedata, filesize), 0);
	//#4 we want to get nonexistent file
	const char * unknown_filename = "this_file_shouldn't_exist_in_the_storage.err";
	filesize = (unsigned)dataOriginal.size();
	ASSERT_EQ(storage->get(unknown_filename, &filedata, &filesize), EC_FALSE);
	utilities::deallocate(filedata);
}

using TestExportParam = TestParamExtra;

INSTANTIATE_TEST_CASE_P(ExportCases, TestExportParam,
	testing::ValuesIn(g_testExportParams));

TEST_P(TestExportParam, CorrectExport)
{
	ParamExtra param = GetParam();
	auto storage = getStorage(param.type, param.initParams);
	ASSERT_TRUE(storage);

	const string path = param.additionParam;
	if (path.empty())
	{
		cout << "Skipping export tests. Export path wasn't set" << endl;
		return;
	}
	string recv_data;

	
	//add elements to db
	for (size_t j = 0; j < g_fileNameData.size(); ++j)
	{
		ASSERT_EQ(storage->add(g_fileNameData[j].first.c_str(), g_fileNameData[j].second.data(),
			g_fileNameData[j].second.size()), OK);
	}

	unsigned lengthBuf = (unsigned)g_fileNameData.size();
	vector<const char*> filenameVec(g_fileNameData.size());
	transform(g_fileNameData.cbegin(), g_fileNameData.cend(), filenameVec.begin(),
		[](const pair<const string&, const string&> & pss)
	{
		return pss.first.c_str();
	});
	const char * const * filenames = filenameVec.data();
	const char * p = path.c_str();

	ASSERT_EQ(storage->exportFiles(filenames, lengthBuf, p), OK);
	//check correctness of exporting files
	for (size_t j = 0; j < lengthBuf; ++j)
	{
		const string pathName = utilities::makePathFile(path, filenameVec[j]);
		ifstream f(pathName, ios::binary | ifstream::in);

		ASSERT_TRUE(f.is_open());
		recv_data = getDataFile(f);
		f.close();
		ASSERT_EQ(g_fileNameData[j].second, recv_data);
	}
}

using TestFullBackupParam = TestParamExtra;

INSTANTIATE_TEST_CASE_P(FullBackupCases, TestFullBackupParam,
	testing::ValuesIn(g_testFullBackupParams));

TEST_P(TestFullBackupParam, CorrectFullBackup)
{
	ParamExtra param = GetParam();
	auto storage = getStorage(param.type, param.initParams);
	ASSERT_TRUE(storage);

	//full backup
	//assert all file are in the storage
	unsigned sizeBuf = g_maxFileSize;
	string buf(g_maxFileSize, 0);
	unsigned amountFiles;


	amountFiles = 0;
	string backupParam = param.additionParam;
	ASSERT_EQ(storage->backupFull(backupParam.c_str(), &amountFiles), OK);

	ASSERT_GE(amountFiles, g_fileNameData.size());
	
	checkFiles(storage.get(), g_fileNameData);

}


using TestIncBackupParam = TestParamExtra;

INSTANTIATE_TEST_CASE_P(IncBackupCases, TestIncBackupParam,
	testing::ValuesIn(g_testIncBackupParams));


TEST_P(TestIncBackupParam, CorrectIncrementalBackup)
{
	ParamExtra param = GetParam();
	auto storage = getStorage(param.type, param.initParams);
	ASSERT_TRUE(storage);


	unsigned amountFiles = 0;
	auto fileToPush = g_fileNameData.front();
	ASSERT_EQ(storage->add(fileToPush.first.c_str(),
		fileToPush.second.data(), fileToPush.second.size()), OK);
	ASSERT_EQ(storage->backupIncremental(param.additionParam.c_str(), &amountFiles), OK);
	ASSERT_GE(amountFiles, 1u);
	checkFiles(storage.get(), g_fileNameData);

	// adding some new files with existing names into original storage
	for (size_t j = 0; j < g_fileNameDataIncBackup.size(); ++j)
	{
		ASSERT_EQ(storage->add(g_fileNameDataIncBackup[j].first.data(),
			g_fileNameDataIncBackup[j].second.data(),
			(unsigned)g_fileNameDataIncBackup[j].second.size()), OK);
	}
	
	amountFiles = 0;
	ASSERT_EQ(storage->backupIncremental(param.additionParam.c_str(), &amountFiles), OK);
	
	if (param.type == e_FTPStorage) //FTP is so special because time is measured in minutes (and sometimes in days!)
	{
		ASSERT_GE(amountFiles, g_fileNameDataIncBackup.size());
	}
	else
	{
		ASSERT_EQ(amountFiles, g_fileNameDataIncBackup.size());
	}
	
	checkFiles(storage.get(), g_fileNameDataIncBackup);
	
}