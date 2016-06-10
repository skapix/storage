#define NOMINMAX
#include "Storage.h"
#include "func.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm> // max
#include <fstream>
#include "..\storage\StringParser.h"

using std::cout;
using std::endl;
using std::string;
using std::ifstream;
using std::ofstream;

string uintToString(const UINT & val)
{
	string ret(30, '\0');
	sprintf_s(&ret[0], 30, "%u", val);
	ret.resize(ret.find('\0'));
	return ret;
}

string doubleToString(const double & val)
{
	string ret(30, '\0');
	sprintf_s(&ret[0], 30, "%f", val);
	ret.resize(ret.find('\0'));
	return ret;
}


double addFunc(Storage * stor, std::istream & stream, const char * filename, HRESULT & retVal, UINT & length)
{
	retVal = E_FAIL;
	//no \,/ in filenames
	const char * name = getFileNameFromPath(filename);
	stream.seekg(0, stream.end);
	length = static_cast<UINT>(stream.tellg());
	stream.seekg(0, stream.beg);
	if (g_maxFileSize < length)
	{
		std::cerr << "Error: file " << filename << " is too large." << endl;
		return -1.;
	}
	string fileBuf(length, '\0');
	stream.read(&fileBuf[0], length);
	double time;
	d_timeCapture(time, retVal = stor->add(name, &fileBuf[0], static_cast<UINT>(length)));
	return time;
}

double getFunc(Storage * stor, const char * key, HRESULT & retVal, string & data)
{
	data.clear();
	retVal = E_FAIL;
	
	double time;
	UINT length = g_maxFileSize;
	string buf(g_maxFileSize,'\0');
	char * s = &buf[0];	
	d_timeCapture(time, retVal = stor->get(key, &s, &length));
	if (length > g_maxFileSize)//file wasn't written in the buf
	{
		cout << "Unexpectedly large file is in storage";
		return -1;
	}
	if (FAILED(retVal) || retVal == S_FALSE)
		return time;

	data.resize(length);
	memcpy_s(&data[0], length, s, length);
	return time;
}


double makeFunc(Storage * stor, const char op, const char ** arguments, const unsigned argument_count, string & output)
{
	HRESULT result = S_OK;
	output.clear();
	double retValTime = -1;
	switch (tolower(op))
	{
	case 'a':
	{
		ifstream f(arguments[0], std::ios::binary | ofstream::in);
		if (!f.is_open())
		{
			cout << "Can't open file " << arguments[0] << endl;
			return -1;//break;
		}
		const char * filename = argument_count > 1 ? arguments[1] : arguments[0];
		UINT lengthFile = -1;
		retValTime = addFunc(stor, f, filename, result, lengthFile);
		if (SUCCEEDED(retValTime))
			output = doubleToString(double(lengthFile)/1024.);
		break;
	}
	case 'g':
	{
		string data;
		retValTime = getFunc(stor, arguments[0], result, data);
		
		if (SUCCEEDED(result) && result != S_FALSE && argument_count>1)
		{
			ofstream f(arguments[1], std::ios::binary | ofstream::out);
			if (!f.is_open())
			{
				cout << "Can't open file " << arguments[1] << endl;
				break;
			}
			f.write(&data[0], data.size());
			f.close();
		}
		if (result == S_OK)
			output = doubleToString(double(data.size()) / 1024.);
		else if (result == S_FALSE)
			output = "-";
		break;
	}
	case 'e':
	{
		d_timeCapture(retValTime, result = stor->exportFiles(arguments, argument_count - 1,
			arguments[argument_count - 1]));
		output.resize(20, '\0');
		
		_itoa_s(argument_count - 1, &output[0], 20, 10);
		output.resize(output.find('\0'));
		break;
	}
	case 'f':
	case 'i':
	{
		
		UINT amount = 0;
		if (tolower(op) == 'f')
		{
			d_timeCapture(retValTime, result = stor->backupFull(arguments[0], &amount));
		}
		else
		{
			d_timeCapture(retValTime, result = stor->backupIncremental(arguments[0], &amount));
		}
		output = uintToString(amount);
		break;
	}
	default:
		cout << "unknown method" << endl;
		return -1.;
	}

	if (FAILED(result))
	{
		cout << "Method failed with code 0x" << std::hex << result << endl;
		return -1.;
	}
	return retValTime;
}




#define d_makeWithQuotes(s) ("\"" + s + "\"")


string mergeServerDbTable(const StringParser & parser)
{
	string ret = d_makeWithQuotes(parser.getServer());
	if (!parser.getDbName().empty())
	{
		ret += string(",") + d_makeWithQuotes(parser.getDbName()) + ',' +
			d_makeWithQuotes(parser.getTableName());
	}
	return ret;
}

//storage,db,table, params[bcp ~ storage/table], elapsed_time, comment([local,global],[hierarhy],[amount backuped])
//params for export files ~ amount of exported files
void makeLogRecord(const char * init, const char * params, const unsigned indexStorage, const char method,
	const double time_elapsed, const char * comment)
{
	if (indexStorage > 6) return;
	const char * methodNames[] = { "FSStorage", "SMBStorage", "FTPStorage", "MsSQLStorage",
		"PostgreSQLStorage", "SQLiteStorage", "MongoDB" };
	string rel_path = methodNames[indexStorage];
	if (!createDir(rel_path))
	{
		cout << "Can't create dir";
		return;
	}
	string method_name, method_params;
	switch (tolower(method))
	{
	case 'a':
		method_name = "add";
		method_params = d_makeWithQuotes(string(params));
		break;
	case 'g':
		method_name = "get";
		method_params = d_makeWithQuotes(string(params));
		break;
	case 'e':
		method_name = "export";
		method_params = d_makeWithQuotes(string(comment));
		comment = nullptr;
		break;
	case 'f':
	case 'i':
	{
		method_name = tolower(method) == 'f' ? "backupFull" : "backupIncremental";
		StringParser parser(params);
		method_params = mergeServerDbTable(parser);
		break;
	}
	default:
		cout << "Can't log unknown method";
		return;
	}
	StringParser parser;
	//for FSStorage
	if (!parser.initialize(init))
		parser.setServer(init);
	//1 column: storage/db/table
	string new_record = mergeServerDbTable(parser) + ',';
	//2 column: params
	new_record += method_params + ',';
	//3 column: time
	string s_time(30,'\0');
	sprintf_s(&s_time[0], 30, "%f", time_elapsed);
	s_time.resize(s_time.find('\0'));
	new_record += s_time;
	//4 column[opt]: comment
	if (comment != nullptr && strlen(comment)>0)
	{
		new_record += ',' + string(comment);
	}

	rel_path = makePathFile(rel_path, method_name + ".csv");
	ofstream f(rel_path, ofstream::app | ofstream::out);
	if (!f.is_open())
	{
		cout << "Can't create/open log file" << endl;
		return;
	}
	f.write(new_record.data(), new_record.size());
	f << endl;

}


void help()
{
	cout << "Following arguments should be passed : " << endl;
	cout << "1 : number of storage to test{0..6}" << endl;
	cout << "2 : initialization(openStorage) data in \" \"" << endl;
	cout << "3 : a method name to test" << endl;
	cout << "4 : parameters for method" << endl;

	cout << endl << "To get additional information use --help and number of point(example: --help 1)" << endl;
	//cout << "5[extra] : Also you can fill storage with random data, created in programm. For looking up info, use -- help 5" << endl;
}

void helpStorage()
{
	cout << "Available storages: " << endl;
	cout << "0 - FSStorage" << endl;
	cout << "1 - SMBStorage" << endl;
	cout << "2 - FTPStorage" << endl;
	cout << "3 - MSSQLStorage" << endl;
	cout << "4 - PostgreSQLStorage" << endl;
	cout << "5 - SQLiteStorage" << endl;
	cout << "6 - MongoDB" << endl;
}

void helpInit()
{
	cout << "Check the documentation to clarify the initializing method" << endl;
}

void helpMethod()
{
	cout << "Available methods : " <<
		"Add, " << "Get, " << "Export, " << "Incremental[backup]" << "Full[backup]" << endl;
	cout << "This methods are parsed by the first letters i.e." << endl <<
		"'a', 'A', 'add', 'Abcd' for add" << endl <<
		"'g', 'G', 'gg' for get" << endl << "..." << endl;
}

void helpParams()
{
	cout << "For add, get you should input only filename[s]."
		"Notice, that you can input several filenames. Method will be executed for each file." << endl;
	cout << "For export, arguments are equivalent for get/add,"
		"but you should add full path of folder, where files will be storaged with the same names." << endl;
	cout << "backup arguments are the similar to initialization arguments" << endl;
}

void helpRandInitStorage()
{
	cout << "Not implemented" << endl;
}



//std::vector<std::pair<double, UINT>> addNFilesIntoStorage(Storage * stor, const unsigned amountFiles,
//	const unsigned minFileSize, const unsigned maxFileSize)
//{
//	std::vector<std::pair<double, UINT>> retVal;
//	RandomGenerator rGen(1);
//	const unsigned filenameBufSize = 20;
//	char filename[filenameBufSize];
//	memchr(filename, 0, filenameBufSize);
//	memcpy(filename, "file", 4);
//	for (size_t i = 0; i < amountFiles; i++)
//	{
//		std::string dat = getRandData(minFileSize, maxFileSize, rGen);
//		_itoa_s(i, filename + 4, filenameBufSize - 4, 10);
//		UINT size = dat.size();
//		double time = -1.;
//		HRESULT res;
//		d_timeCapture(time, res = stor->add(filename, dat.data(), size));
//		if (FAILED(res))
//		{
//			retVal.push_back(std::make_pair(time, 0));
//		}
//		else
//		{
//			retVal.push_back(std::make_pair(time, size));
//		}
//	}
//	return retVal;
//}

