#ifndef _XALLOCATOR_H
#define _XALLOCATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void xalloc_init(void);
void xalloc_destroy(void);
void* xmalloc(size_t size);
void xfree(void* ptr);
void* xrealloc(void* ptr, size_t size);
void xalloc_stats(void);

#ifdef __cplusplus
}
#endif

#endif
