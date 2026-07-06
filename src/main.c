// chrlauncher
// Copyright (c) 2015-2026 Henry++

#include "routine.h"

#include "main.h"
#include "app_paths.h"
#include "app_config.h"
#include "app_dns.h"
#include "app_extensions.h"
#include "patch_arena.h"
#include "patch_arena.c"
#include "rapp.h"

#include "CpuArch.h"

#include "7z.h"
#include "7zAlloc.h"
#include "7zBuf.h"
#include "7zCrc.h"
#include "7zFile.h"
#include "7zWindows.h"

#include "miniz.h"

#include "resource.h"

#include <shlobj.h>
#include <shobjidl.h>


BROWSER_INFORMATION browser_info = {0};

R_QUEUED_LOCK lock_download = PR_QUEUED_LOCK_INIT;
R_QUEUED_LOCK lock_thread = PR_QUEUED_LOCK_INIT;

R_WORKQUEUE workqueue;
