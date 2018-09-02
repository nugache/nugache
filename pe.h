#include <windows.h>

// Must supply NtHeaders as a pointer to a buffer containing entire header to read sections
DWORD RvaToVa(IMAGE_NT_HEADERS* NtHeaders, DWORD Rva);