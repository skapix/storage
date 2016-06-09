#pragma once
#include <chrono>
#include "Storage.h"
#include <iosfwd>
#include "../storage/auxiliaryStorage.h"//some additional info

//<0 if failed, time otherwise
//output ~ comment
double makeFunc(Storage * stor, const char op, const char ** arguments, const unsigned argument_count, std::string & output);
void makeLogRecord(const char * init, const char * params, const unsigned indexStorage, const char method,
	const double time_elapsed, const char * comment);

void help();
void helpStorage();
void helpInit();
void helpMethod();
void helpParams();
void helpRandInitStorage();

//std::vector<std::pair<double, UINT>> addNFilesIntoStorage(Storage * stor, const unsigned amountFiles,
//	const unsigned minFileSize = g_minFileSize, const unsigned maxFileSize = g_maxFileSize);


#define d_timeCapture(time,func)\
	{\
	std::chrono::high_resolution_clock::time_point __t1 = std::chrono::high_resolution_clock::now();\
	func;\
	std::chrono::high_resolution_clock::time_point __t2 = std::chrono::high_resolution_clock::now();\
	time = std::chrono::duration_cast<std::chrono::duration<double>>(__t2 - __t1).count();\
	}