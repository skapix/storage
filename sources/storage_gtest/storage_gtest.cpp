#include "tests.h"
#include "auxiliary.h" //g_amountClasses
#include <iostream>

using namespace std;

string uintToString(const UINT val)
{
	string ret(30, '\0');
	sprintf_s(&ret[0], 30, "%u", val);
	ret.resize(ret.find('\0'));
	return ret;
}

int main(int argc, char * argv[])
{
	if (argc > 2 && strcmp(argv[1], "--help") == 0 || argc == 1)
	{
		cout << "Take a look at example [example_gtest.txt] in the root dir" << endl;
		return 0;
	}
	for (int i = 1; i < argc;)
	{
		char cnum = argv[i][0];
		int num = -1;
		if (cnum >= '0' && cnum <= '9')
		{
			num = atoi(argv[i]);
			if (num < 0 || num >= g_amountClasses)
			{
				cout << "Wrong number: " << num << ". Number should be in range of amount classes" << endl;
				++i;
				continue;
			}
		}
		else
		{
			cout << "Can't parse argument: " << argv[i] << ". Number should be presented" << endl;
			++i;
			continue;
		}
		// num is correct. It's time to get init params.
		++i;
		if (i >= argc) break;
		{
			map<string, string> s;
			s[im::g_id] = uintToString((UINT)num);
			// don't check init params, they are without flags
			s[im::g_init] = argv[i];
			g_inits.push_back(std::move(s));
		}
		++i;
		if (i >= argc) break;

		for (int k = 0; k < 2; ++k)
		{
			if (strcmp(argv[i], "-f") == 0)
			{
				++i;
				if (i >= argc) break;
				g_inits.back()[im::g_fb] = argv[i];
				++i;
			}
			else if (strcmp(argv[i], "-i") == 0)
			{
				++i;
				if (i >= argc) break;
				g_inits.back()[im::g_ib] = argv[i];
				++i;
			}
			else
				break;
		}
	}

	int no_argc = 1;
	::testing::InitGoogleTest(&no_argc, argv);
	int retVal = RUN_ALL_TESTS();
	system("pause");
	return retVal;

}
