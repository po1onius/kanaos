[bits 32]

extern kernel_init
extern console_init
extern memory_init
extern device_init


global _start
_start:
push ebx
push eax
call device_init
call console_init
call memory_init
call kernel_init
jmp $
