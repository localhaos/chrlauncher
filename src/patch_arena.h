// chrlauncher
// Patch/hook memory arena facade.

#pragma once

#include "main.h"

BOOLEAN patch_arena_initialize (
	_In_ SIZE_T arena_size
);

VOID patch_arena_destroy ();

PVOID patch_arena_alloc (
	_In_ SIZE_T size
);

PVOID patch_arena_alloc_aligned (
	_In_ SIZE_T size,
	_In_ SIZE_T alignment
);

VOID patch_arena_free (
	_In_opt_ PVOID pointer
);

VOID patch_arena_reset ();

BOOLEAN patch_arena_is_enabled ();
