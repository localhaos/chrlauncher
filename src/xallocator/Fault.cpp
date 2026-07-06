#include "Fault.h"
#include "DataTypes.h"
#include <assert.h>

void FaultHandler(const char* file, unsigned short line)
{
	(void)file;
	(void)line;
	assert(0);
}
