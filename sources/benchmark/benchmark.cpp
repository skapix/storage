#include <initguid.h>
#include "boost\scope_exit.hpp"
#include <fstream>
#include <iostream>
#include "func.h"
#include <string>

using std::cout;
using std::endl;
using std::ifstream;
using std::string;

int main(int argc, char * argv[])
{
	if (argc < 3)
	{
		help();
		return 0;
	}
	if (strcmp(argv[1], "--help") == 0)
	{
		int h = argv[2][0] - '0';
		if (h >= 0 && h <= 5)
		{
			typedef void(*Func)();
			const Func funcs[5] = { helpStorage, helpInit, helpMethod, helpParams, helpRandInitStorage };
			funcs[h]();
		}
		else
		{
			help();
		}
		return 0;
	}
	//0 - executable full name
	//1 - storage type
	//2 - init arg
	//3 - method name
	//4+ - method param[s]
	if (argc < 5 || argv[1][0] < '0' || argv[1][0] > '6')
	{
		help();
		return 0;
	}
	
	unsigned numberStorage = argv[1][0] - '0';
	const Storage_t iidClasses[] = { e_FSStorage, e_SMBStorage, e_FTPStorage,
		e_MSSQLStorage, e_PostgreSQL, e_SQLite3, e_MongoDB };

	 Storage * stor = createStorage(iidClasses[numberStorage]);
	if (stor == nullptr)
		return 1;
	BOOST_SCOPE_EXIT(stor) {
		releaseStorage(stor);
	}BOOST_SCOPE_EXIT_END

	ErrorCode rc = stor->openStorage(argv[2]);
	if (failed(rc))
	{
		cout << "Can't initialize(open) storage" << endl;
		return 1;
	}

	char method = tolower(argv[3][0]);
	const char ** argv_pointer = const_cast<const char**>(argv + 4);
	string comment;
	double time_measurement = makeFunc(stor, method, argv_pointer, argc - 4, comment);
	if (time_measurement < 0)
	{
		cout << "Wrong time measurement" << endl;
		return 1;
	}
	//argv[4] in exportFiles method doesn't count
	makeLogRecord(argv[2], argv[4], numberStorage, method, time_measurement, comment.c_str());

	return 0;
}