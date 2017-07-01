enum ErrorCode
{
  OK                  =  0,
  EC_FALSE            =  1,
	FAILEDTOGETWINDIR   = -1,
	OUTOFMEMORY         = -2,
	FAILEDTOCREATEFILE  = -3,
	INVALIDARG          = -4,
	CANTCOPYFILE        = -5,
	UNEXPECTED          = -6,
	INSUFFICIENTMEMORY  = -7,
	LOG_CORRUPTED       = -8,
	SYSTEMAPP           = -9,
	FILE_READFAIL       = -10,
	FILE_WRITEFAIL      = -11,
	FAIL                = -12,
	FILE_TOO_LARGE      = -13,
	INVALID_DATA        = -14
};


inline bool failed(const ErrorCode err)
{
	return err < 0;
}

inline bool succeeded(const ErrorCode err)
{
	return err >= 0;
}

//std::string getECDescription(const ErrorCode err);