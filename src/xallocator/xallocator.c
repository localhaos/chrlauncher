#include "xallocator.h"

#include <windows.h>
#include <stdint.h>

#define XALLOC_MAGIC 0x58414C4Cu

typedef struct _XALLOC_HEADER
{
	uint32_t magic;
	uint32_t bucket;
	size_t requested;
	size_t capacity;
	struct _XALLOC_HEADER* next;
} XALLOC_HEADER;

typedef struct _XALLOC_BUCKET
{
	size_t capacity;
	XALLOC_HEADER* free_list;
	uint64_t alloc_count;
	uint64_t free_count;
	uint64_t reuse_count;
} XALLOC_BUCKET;

static CRITICAL_SECTION g_xalloc_lock;
static volatile LONG g_xalloc_ready = 0;
static XALLOC_BUCKET g_xalloc_buckets[] = {
	{ 16, 0, 0, 0, 0 },
	{ 32, 0, 0, 0, 0 },
	{ 64, 0, 0, 0, 0 },
	{ 128, 0, 0, 0, 0 },
	{ 256, 0, 0, 0, 0 },
	{ 512, 0, 0, 0, 0 },
	{ 1024, 0, 0, 0, 0 },
	{ 2048, 0, 0, 0, 0 },
	{ 4096, 0, 0, 0, 0 },
	{ 8192, 0, 0, 0, 0 },
	{ 16384, 0, 0, 0, 0 },
	{ 32768, 0, 0, 0, 0 },
};

static void xalloc_ensure_init(void)
{
	if (InterlockedCompareExchange(&g_xalloc_ready, 1, 0) == 0)
		InitializeCriticalSection(&g_xalloc_lock);

	while (g_xalloc_ready != 1)
		Sleep(0);
}

void xalloc_init(void)
{
	xalloc_ensure_init();
}

static int xalloc_find_bucket(size_t size)
{
	for (int i = 0; i < (int)(sizeof(g_xalloc_buckets) / sizeof(g_xalloc_buckets[0])); i++)
	{
		if (size <= g_xalloc_buckets[i].capacity)
			return i;
	}

	return -1;
}

void* xmalloc(size_t size)
{
	XALLOC_HEADER* header;
	int bucket;
	size_t capacity;
	size_t total;

	if (size == 0)
		size = 1;

	xalloc_ensure_init();

	bucket = xalloc_find_bucket(size);
	capacity = (bucket >= 0) ? g_xalloc_buckets[bucket].capacity : size;
	total = sizeof(XALLOC_HEADER) + capacity;

	EnterCriticalSection(&g_xalloc_lock);

	if (bucket >= 0 && g_xalloc_buckets[bucket].free_list)
	{
		header = g_xalloc_buckets[bucket].free_list;
		g_xalloc_buckets[bucket].free_list = header->next;
		g_xalloc_buckets[bucket].reuse_count++;
	}
	else
	{
		header = (XALLOC_HEADER*)HeapAlloc(GetProcessHeap(), 0, total);
	}

	if (bucket >= 0)
		g_xalloc_buckets[bucket].alloc_count++;

	LeaveCriticalSection(&g_xalloc_lock);

	if (!header)
		return NULL;

	header->magic = XALLOC_MAGIC;
	header->bucket = (uint32_t)bucket;
	header->requested = size;
	header->capacity = capacity;
	header->next = NULL;

	return (void*)(header + 1);
}

void xfree(void* ptr)
{
	XALLOC_HEADER* header;
	int bucket;

	if (!ptr)
		return;

	header = ((XALLOC_HEADER*)ptr) - 1;

	if (header->magic != XALLOC_MAGIC)
	{
		HeapFree(GetProcessHeap(), 0, header);
		return;
	}

	bucket = (int)header->bucket;

	xalloc_ensure_init();
	EnterCriticalSection(&g_xalloc_lock);

	if (bucket >= 0 && bucket < (int)(sizeof(g_xalloc_buckets) / sizeof(g_xalloc_buckets[0])))
	{
		header->next = g_xalloc_buckets[bucket].free_list;
		g_xalloc_buckets[bucket].free_list = header;
		g_xalloc_buckets[bucket].free_count++;
	}
	else
	{
		HeapFree(GetProcessHeap(), 0, header);
	}

	LeaveCriticalSection(&g_xalloc_lock);
}

void* xrealloc(void* ptr, size_t size)
{
	XALLOC_HEADER* header;
	void* new_ptr;
	size_t copy_size;

	if (!ptr)
		return xmalloc(size);

	if (size == 0)
	{
		xfree(ptr);
		return NULL;
	}

	header = ((XALLOC_HEADER*)ptr) - 1;

	if (header->magic == XALLOC_MAGIC && size <= header->capacity)
	{
		header->requested = size;
		return ptr;
	}

	new_ptr = xmalloc(size);

	if (!new_ptr)
		return NULL;

	copy_size = (header->magic == XALLOC_MAGIC && header->requested < size) ? header->requested : size;
	CopyMemory(new_ptr, ptr, copy_size);
	xfree(ptr);

	return new_ptr;
}

void xalloc_stats(void)
{
	/* intentionally silent in Windows GUI builds */
}

void xalloc_destroy(void)
{
	XALLOC_HEADER* header;
	XALLOC_HEADER* next;

	if (g_xalloc_ready != 1)
		return;

	EnterCriticalSection(&g_xalloc_lock);

	for (int i = 0; i < (int)(sizeof(g_xalloc_buckets) / sizeof(g_xalloc_buckets[0])); i++)
	{
		header = g_xalloc_buckets[i].free_list;
		g_xalloc_buckets[i].free_list = NULL;

		while (header)
		{
			next = header->next;
			HeapFree(GetProcessHeap(), 0, header);
			header = next;
		}
	}

	LeaveCriticalSection(&g_xalloc_lock);
	DeleteCriticalSection(&g_xalloc_lock);
	g_xalloc_ready = 0;
}
