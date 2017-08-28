// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <WtsApi32.h>
#include <Winternl.h>
#include <cstdio>
#include <tlhelp32.h>
#include <comdef.h>
#include <psapi.h>
#include <Winternl.h>
#include <Wtsapi32.h>                                                                                          
#include <stdlib.h>

#ifdef _DEBUG 
#undef free
#define free(p) _free_dbg(p, _NORMAL_BLOCK); *(int*)&p = 0x666;
#endif

#ifndef _DEBUG_ONE
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib,"Wtsapi32.lib")
#pragma comment(lib,"RpcRT4.lib")
#endif

#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        102
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#endif
#endif
#include <stdint.h>

typedef int(*FUNKY_POINTER)(void);

#ifdef __cplusplus
#include <SDKDDKVer.h>
#endif

#define _WIN32_WINNT_WIN10_TH2 _WIN32_WINNT_WIN10
#define _WIN32_WINNT_WIN10_RS1 _WIN32_WINNT_WIN10

#ifdef _UNICODE
typedef wchar_t TCHAR;
#else
typedef char TCHAR;
#endif 
typedef const TCHAR* LPCTSTR;

#ifndef _WIN32_WINNT 
#define _WIN32_WINNT 0x0600     
#endif

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//




// TODO: reference additional headers your program requires here

// TODO: reference additional headers your program requires here

