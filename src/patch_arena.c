// chrlauncher
// Optional EasyMem-backed arena for future hook/patch metadata.

#include "patch_arena.h"

#if defined(__has_include)
#  if __has_include("easy_memory/easy_memory.h")
#    define CHRLAUNCHER_HAVE_EASY_MEMORY 1
#  endif
#endif

#if defined(CHRLAUNCHER_HAVE_EASY_MEMORY)
#  define EM_STATIC
#  define EASY_MEMORY_IMPLEMENTATION
#  include "easy_memory/easy_memory.h"
#endif

typedef struct _PATCH_ARENA_FALLBACK_HEADER
{
	SIZE_T size;
	SIZE_T offset;
} PATCH_ARENA_FALLBACK_HEADER, *PPATCH_ARENA_FALLBACK_HEADER;

static R_QUEUED_LOCK patch_arena_lock = PR_QUEUED_LOCK_INIT;

#if defined(CHRLAUNCHER_HAVE_EASY_MEMORY)
static EM* patch_arena_em = NULL;
#else
static PBYTE patch_arena_buffer = NULL;
static SIZE_T patch_arena_capacity = 0;
static SIZE_T patch_arena_offset = 0;
#endif

static SIZE_T patch_arena_align_up (
	_In_ SIZE_T value,
	_In_ SIZE_T alignment
)
{
	if (alignment == 0)
		alignment = sizeof (PVOID);

	if ((alignment & (alignment - 1)) != 0)
		return value;

	return (value + alignment - 1) & ~(alignment - 1);
}

BOOLEAN patch_arena_initialize (
	_In_ SIZE_T arena_size
)
{
	BOOLEAN result = FALSE;

	if (!arena_size)
		arena_size = 1024 * 1024;

	_r_queuedlock_acquireshared (&patch_arena_lock);

#if defined(CHRLAUNCHER_HAVE_EASY_MEMORY)
	if (!patch_arena_em)
		patch_arena_em = em_create (arena_size);

	result = (patch_arena_em != NULL);
#else
	if (!patch_arena_buffer)
	{
		patch_arena_buffer = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, arena_size);
		patch_arena_capacity = patch_arena_buffer ? arena_size : 0;
		patch_arena_offset = 0;
	}

	result = (patch_arena_buffer != NULL);
#endif

	_r_queuedlock_releaseshared (&patch_arena_lock);

	return result;
}

VOID patch_arena_destroy ()
{
	_r_queuedlock_acquireshared (&patch_arena_lock);

#if defined(CHRLAUNCHER_HAVE_EASY_MEMORY)
	if (patch_arena_em)
	{
		em_destroy (patch_arena_em);
		patch_arena_em = NULL;
	}
#else
	if (patch_arena_buffer)
	{
		HeapFree (GetProcessHeap (), 0, patch_arena_buffer);
		patch_arena_buffer = NULL;
		patch_arena_capacity = 0;
		patch_arena_offset = 0;
	}
#endif

	_r_queuedlock_releaseshared (&patch_arena_lock);
}

PVOID patch_arena_alloc_aligned (
	_In_ SIZE_T size,
	_In_ SIZE_T alignment
)
{
	PVOID result = NULL;

	if (!size)
		return NULL;

	if (!patch_arena_is_enabled () && !patch_arena_initialize (1024 * 1024))
		return NULL;

	_r_queuedlock_acquireshared (&patch_arena_lock);

#if defined(CHRLAUNCHER_HAVE_EASY_MEMORY)
	result = em_alloc_aligned (patch_arena_em, size, alignment ? alignment : sizeof (PVOID));
#else
	{
		SIZE_T header_offset;
		SIZE_T data_offset;
		SIZE_T next_offset;
		PPATCH_ARENA_FALLBACK_HEADER header;

		header_offset = patch_arena_align_up (patch_arena_offset, sizeof (PVOID));
		data_offset = patch_arena_align_up (header_offset + sizeof (PATCH_ARENA_FALLBACK_HEADER), alignment ? alignment : sizeof (PVOID));
		next_offset = data_offset + size;

		if (patch_arena_buffer && next_offset <= patch_arena_capacity)
		{
			header = (PPATCH_ARENA_FALLBACK_HEADER)(patch_arena_buffer + data_offset - sizeof (PATCH_ARENA_FALLBACK_HEADER));
			header->size = size;
			header->offset = header_offset;
			result = patch_arena_buffer + data_offset;
			patch_arena_offset = next_offset;
		}
	}
#endif

	_r_queuedlock_releaseshared (&patch_arena_lock);

	return result;
}

PVOID patch_arena_alloc (
	_In_ SIZE_T size
)
{
	return patch_arena_alloc_aligned (size, sizeof (PVOID));
}

VOID patch_arena_free (
	_In_opt_ PVOID pointer
)
{
	if (!pointer)
		return;

	_r_queuedlock_acquireshared (&patch_arena_lock);

#if defined(CHRLAUNCHER_HAVE_EASY_MEMORY)
	em_free (pointer);
#else
	/* fallback arena is reset/destroy only */
	UNREFERENCED_PARAMETER (pointer);
#endif

	_r_queuedlock_releaseshared (&patch_arena_lock);
}

VOID patch_arena_reset ()
{
	_r_queuedlock_acquireshared (&patch_arena_lock);

#if defined(CHRLAUNCHER_HAVE_EASY_MEMORY)
	if (patch_arena_em)
		em_reset (patch_arena_em);
#else
	patch_arena_offset = 0;

	if (patch_arena_buffer && patch_arena_capacity)
		RtlSecureZeroMemory (patch_arena_buffer, patch_arena_capacity);
#endif

	_r_queuedlock_releaseshared (&patch_arena_lock);
}

BOOLEAN patch_arena_is_enabled ()
{
#if defined(CHRLAUNCHER_HAVE_EASY_MEMORY)
	return patch_arena_em != NULL;
#else
	return patch_arena_buffer != NULL;
#endif
}
