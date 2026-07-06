#pragma once

#include "main.h"

typedef struct _CFGREAD CFGREAD, *PCFGREAD;

PCFGREAD cfgread_open (
	_In_ PR_STRING path
);

VOID cfgread_close (
	_In_opt_ PCFGREAD file
);

LPCWSTR cfgread_get (
	_In_opt_ PCFGREAD file,
	_In_opt_ LPCWSTR section_name,
	_In_ LPCWSTR key_name
);
