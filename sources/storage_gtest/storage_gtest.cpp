#include "tests.h"
#include "auxiliary.h" //g_amountClasses

int main(int argc, char * argv[])
{
	if (argc >= 2)
	{
		char c = argv[1][0];
		if (c >= '0' && c <= '9')
		{
			g_numToTest = c - '0';
			if (g_numToTest < g_amountClasses)
				g_soloTest = true;
		}
	}
	::testing::InitGoogleTest(&argc, argv);
	int retVal = RUN_ALL_TESTS();
	system("pause");
	return retVal;
	
}
