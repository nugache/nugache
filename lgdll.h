#include <windows.h>
#include "config.h"
#include "file.h"

#ifdef _EXPORTING
   #define DECLSPEC __declspec(dllexport)
#else
   #define DECLSPEC __declspec(dllimport)
#endif

VOID DECLSPEC InstallHook(VOID);
VOID DECLSPEC RemoveHook(VOID);