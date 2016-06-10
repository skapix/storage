#pragma once
#include "Storage.h"
#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <map>

// init mapping
namespace im
{
	const std::string g_id("id"),
		g_init("orig"),
		g_fb("fb"),
		g_ib("ib");
}



// id -> number 0..6
// init -> initialization params
// fb -> full backup params
// ib -> incremental backup params
extern std::vector<std::map<std::string, std::string>> g_inits;