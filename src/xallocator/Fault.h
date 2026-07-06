#ifndef _FAULT_H
#define _FAULT_H

#ifdef __cplusplus
extern "C" {
#endif

#define ASSERT() \
	FaultHandler(__FILE__, (unsigned short) __LINE__)

#define ASSERT_TRUE(condition) \
	do {if (!(condition)) FaultHandler(__FILE__, (unsigned short) __LINE__);} while (0)

void FaultHandler(const char* file, unsigned short line);

#ifdef __cplusplus
}
#endif

#endif
