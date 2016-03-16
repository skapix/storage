#include "StringParser.h"
#include <algorithm>
#include <cstring>

using namespace std;


const std::string g_month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
//enum class e_NameId { Server, Port, Login, Password, DatabaseName, TableName, NoName };

const char * g_serverNames[] = { "serv", "server", "hostaddr" };
const char * g_portNames[] = { "port" };
const char * g_loginNames[] = { "login", "log", "user", "uid" };
const char * g_passwdNames[] = { "password", "passwd", "pwd" };
const char * g_dbNames[] = { "dbname", "db", "database" };
const char * g_tableNames[] = { "tablename", "table", "collection"};

//const map<const char *, e_NameId> mapNames = {
//	{ g_serverNames[0], e_NameId::Server }, { g_serverNames[1], e_NameId::Server }, { g_serverNames[2], e_NameId::Server },
//	{ g_portNames[0], e_NameId::Port },
//	{ g_loginNames[0], e_NameId::Login }, { g_loginNames[1], e_NameId::Login }, { g_loginNames[2], e_NameId::Login },
//	{ g_passwdNames[0], e_NameId::Password }, { g_passwdNames[1], e_NameId::Password }, { g_passwdNames[2], e_NameId::Password },
//	{ g_dbNames[0], e_NameId::DatabaseName }, { g_dbNames[1], e_NameId::DatabaseName }, { g_dbNames[2], e_NameId::DatabaseName },
//	{ g_tableNames[0], e_NameId::TableName }, { g_tableNames[0], e_NameId::TableName }
//};


inline bool consistString(const char * name, const char ** names, const unsigned amountNames)
{
	for (unsigned i = 0; i < amountNames; i++)
	{
		if (strcmp(name, names[i]) == 0)
			return true;
	}
	return false;
}

const char * delims = " =,;";

//-D_SCL_SECURE_NO_WARNINGS
bool StringParser::initialize(const char * st)
{
	
	char * token = NULL, *next_token = NULL;
	string aux_string(st);
	token = strtok_s(&aux_string[0], delims, &next_token);
	while (token != NULL)
	{
		std::transform(token, token + strlen(token), token, tolower);
		string * name = nullptr;
		if (consistString(token, g_serverNames, sizeof(g_serverNames) / sizeof(char*)))
			name = &this->server;
		else if (consistString(token, g_portNames, sizeof(g_portNames) / sizeof(char*)))
			name = &this->port;
		else if (consistString(token, g_loginNames, sizeof(g_loginNames) / sizeof(char*)))
			name = &this->login;
		else if (consistString(token, g_passwdNames, sizeof(g_passwdNames) / sizeof(char*)))
			name = &this->password;
		else if (consistString(token, g_dbNames, sizeof(g_dbNames) / sizeof(char*)))
			name = &this->dbName;
		else if (consistString(token, g_tableNames, sizeof(g_tableNames) / sizeof(char*)))
			name = &this->tableName;
		else
			return false;
			
		token = strtok_s(NULL, delims, &next_token);
		name->assign(token);
		token = strtok_s(NULL, delims, &next_token);
	}
	return true;
}






//////////////////////////////////////////////////////////////////


bool isNumber(const char c)
{
	return c <= '9' && c >= '0';
}

bool isTime(const string & st, const size_t pos)
{
	if (pos < 2 || pos > st.size()-3) return false;
	return isNumber(st[pos - 1]) && isNumber(st[pos - 2]) && isNumber(st[pos + 1]) && isNumber(st[pos + 2]);
}

FTPLsLine soloParseFTPLs(const string & soloServerResponse)
{
	FTPLsLine res;
	switch (soloServerResponse[0])
	{
	case '-': 
		res.typeFile = res.FILE;
		break;
	case 'd':
		res.typeFile = res.DIR;
		break;
	default:
		res.typeFile = res.OTHER;
		break;
	}
	size_t monthPos = 0;
	for (size_t i = 0; i < 12; i++)
	{
		size_t aux = find_end(soloServerResponse.begin(), soloServerResponse.end(), g_month[i].begin(), g_month[i].end())
			- soloServerResponse.begin();
		if (aux != soloServerResponse.size())
			monthPos = max(monthPos, aux);
	}
	//unknown response format
	if (monthPos == 0 || monthPos + 13 >= soloServerResponse.size())
		return res;
	res.modifiedDate.assign(soloServerResponse.begin() + monthPos, soloServerResponse.begin() + monthPos + 12);
	res.filename.assign(soloServerResponse.begin() + monthPos + 13, soloServerResponse.end());
	return res;
}


vector<FTPLsLine> parseFTPLs(const string & serverResponse)
{
	vector<FTPLsLine> res;
	if (serverResponse.empty())
		return res;
	size_t first = 0;
	while (first != serverResponse.size())
	{
		size_t second = serverResponse.find_first_of("\r\n", first);
		second = second == string::npos ? serverResponse.size() : second;
		if (second - first > 1)
		{
			string line(serverResponse.begin() + first, serverResponse.begin() + second);
			res.push_back(soloParseFTPLs(line));
		}
		first = second+1;
	}
	return res;
}
