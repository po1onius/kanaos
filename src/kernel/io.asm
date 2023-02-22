[bits 32]

section .text


;显卡用的寄存器和cpu访问内存比较相似
;像地址寄存器输入一个地址
;从数据寄存器中取值
;0x3d4 地址寄存器 
;0x3d5 数据寄存器 
;0xe 此地址访问光标高8位 
;0xf 此地址访问光标低8位 

global inb
inb:
push ebp
mov ebp, esp
xor eax, eax
mov edx, [ebp + 8]
in al, dx
leave
ret

global inw
inw:
push ebp
mov ebp, esp
xor eax, eax
mov edx, [ebp + 8]
in ax, dx
leave
ret


global outb
outb:
push ebp
mov ebp, esp
mov edx, [ebp + 8]
mov eax, [ebp + 12]
out dx, al
leave
ret

global outw
outw:
push ebp
mov ebp, esp
mov edx, [ebp + 8]
mov eax, [ebp + 12]
out dx, ax
leave
ret