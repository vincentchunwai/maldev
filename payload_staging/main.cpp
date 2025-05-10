#include <windows.h>
#include <wininet.h> 
#include <cstdio>

using InternetOpenW = HINTERNET(WINAPI*)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
using InternetOpenUrlW = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, LPCWSTR, DWORD, DWORD, DWORD_PTR);
using InternetReadFile = BOOL(WINAPI*)(HINTERNET, LPVOID, DWORD, LPDWORD);



int main() {
    HMODULE hModule = GetModuleHandleW(L"wininet.dll");
    if (hModule == NULL) {
        wprintf(L"[!] GetModuleHandleW() Failed with Error: %d\n", GetLastError());
        return -1;
    }

    InternetOpenW pInternetOpenW = reinterpret_cast<InternetOpenW>(GetProcAddress(hModule, "InternetOpenW"));
    

   
}