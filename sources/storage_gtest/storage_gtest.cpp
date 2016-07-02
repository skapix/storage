#include "tests.h"
#include "auxiliary.h"
#include "storage/StringParser.h"
#include <iostream>
#include <boost/algorithm/string/join.hpp>
#include <boost/scope_exit.hpp>

using namespace std;


int main(int argc, char * argv[])
{
	if (argc > 2 && strcmp(argv[1], "--help") == 0 || argc == 1)
	{
		cout << "Take a look at example [example_gtest.txt] in the root dir" << endl <<
			"If you want to pass additional parameters to gtest, pass them at first";
		return 0;
	}

	int i = 1;
	while (argv[i][0] == argv[i][1] && argv[i][1] == '-')
	{
		++i;
	}

	int totalGtestParams = i;

	for (; i < argc;)
	{
		ParamExtra currentParam;
		string storageShortcut = toUpper(argv[i]);
		if (!g_shortcuts.count(storageShortcut))
		{
			cout << "Can't find storage: " << argv[i] << endl;
			++i;
			continue;
		}

		currentParam.type = g_shortcuts.find(storageShortcut)->second;

		// get params
		++i;
		if (i >= argc)
		{
			cout << "Initialization parameters weren't found" << endl;
			break;
		}

		// don't check init params, should be without flags
		currentParam.initParams = argv[i];

		g_testSimpleParams.emplace_back(InitParam{ currentParam.type, currentParam.initParams });

		++i;
		if (i >= argc) break;

		auto addTest = [&](std::vector<ParamExtra>& param)
		{
			++i;
			if (i >= argc) 
				return;
			currentParam.additionParam = argv[i];
			param.push_back(currentParam);
			++i;
		};

		for (int k = 0; k < 3 && i < argc; ++k)
		{
			if (strcmp(argv[i], "-e") == 0)
			{
				addTest(g_testExportParams);
			}
			else if (strcmp(argv[i], "-if") == 0 ||
				strcmp(argv[i], "-fi") == 0)
			{
				addTest(g_testFullBackupParams);
				g_testIncBackupParams.push_back(g_testFullBackupParams.back());
			}
			else if (strcmp(argv[i], "-f") == 0)
			{
				addTest(g_testFullBackupParams);
			}
			else if (strcmp(argv[i], "-i") == 0)
			{
				addTest(g_testFullBackupParams);
			}
			else
				break;
		}
	}

	for (size_t i = 0; i < g_fileNameData.size(); ++i)
	{
		g_fileNameData[i].second = getRandData(g_minFileSize, g_maxFileSize, RandomGenerator(i));
	}

	for (size_t i = 0; i < g_fileNameDataIncBackup.size(); ++i)
	{
		g_fileNameDataIncBackup[i].second = getRandData(g_minFileSize, g_maxFileSize,
			RandomGenerator(-1 - i));
	}

	::testing::InitGoogleTest(&totalGtestParams, argv);
	int retVal = RUN_ALL_TESTS();
	//system("pause");
	return retVal;

}
