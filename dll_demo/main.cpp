#include <windows.h>
#include <iostream>

typedef void (WINAPI* HelloWorldFunctionPointer)();

void call() {
    const char* dllPath = "C:\\Users\\cheun\\source\\repos\\SampleDLL\\x64\\Debug\\SampleDLL.dll"; // Replace with the path to your DLL

    // Load the DLL
    HMODULE hModule = LoadLibraryA(dllPath);
    if (hModule == NULL) {
        hModule = LoadLibraryA("SampleDLL.dll"); // Try loading from the current directory
    }

    FARPROC pHelloWorld = GetProcAddress(hModule, "HelloWorld");

    if (pHelloWorld == NULL) {
        std::cerr << "Failed to get function address: " << GetLastError() << std::endl;
        FreeLibrary(hModule); // Free the library if we fail to get the function address
        return;
    }

    HelloWorldFunctionPointer HelloWorld = (HelloWorldFunctionPointer)pHelloWorld;

    HelloWorld();

    FreeLibrary(hModule); // Free the library after use
}


int main() {
    
    call();

    return 0;
}