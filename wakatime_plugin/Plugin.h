#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <windows.h>
#include <fstream>
#include <sstream>
#include <memory>
#include <Psapi.h>
#include "Shlwapi.h"

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Version.lib")

#include "ReClassAPI.h"
#include "Wakatime.h"

BOOL
PLUGIN_CC
WriteCallback(
    IN LPVOID Address,
    IN LPVOID Buffer,
    IN SIZE_T Size,
    OUT PSIZE_T BytesWritten
);

BOOL
PLUGIN_CC
ReadCallback(
    IN LPVOID Address,
    IN LPVOID Buffer,
    IN SIZE_T Size,
    OUT PSIZE_T BytesRead
);
