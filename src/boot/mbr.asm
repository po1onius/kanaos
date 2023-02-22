[org 0x7c00]

;初始化段寄存器，有的环境下，段寄存器不初始化会报错
mov ax, 0  
mov ds, ax
mov es, ax
mov ss, ax
mov fs, ax
mov sp, 0x7c00

;bios 0x10号中断
;ax=3 子功能 清屏
mov ax, 3 
int 0x10
mov si, str_mbr
call print 


;端口号作用
;0x1F0: 16bit,用于读写数据
;0x1F1: 检测前一个指令的错误
;0x1F2: 读写扇区的数量
;0x1F3: 起始扇区的0~7位
;0x1F4: 起始扇区的8~15位
;0x1F5: 起始扇区的16~23位
;0x1F6: 
  ;0~3: 起始扇区的24~27位
  ;4: 0 主盘 / 1 从盘
  ;5: 固定为1
  ;6: 0 CHS / 1 LBA
  ;7: 固定为1
;0x1F7:
  ;out:
    ;0xEC: 识别硬盘
    ;0x20: 读硬盘
    ;0x30: 写硬盘
  ;in(8bit)
    ;0: ERR
    ;3: DRQ 数据准备完毕
    ;7: BSY 硬盘busy

;从磁盘读取loader
mov edi, 0x1000 ;读到这个位置
mov ecx, 2 ;读取的起始扇区
mov bl, 4 ;读取的扇区数量
call read_disk
jmp 0x1000 ;跳转到loader


read_disk:
mov dx, 0x1f2
mov al, bl
out dx, al

mov dx, 0x1f3
mov al, cl
out dx, al

mov dx, 0x1f4
shr ecx, 8
mov al, cl
out dx, al

mov dx, 0x1f5
shr ecx, 8
mov al, cl
out dx, al

mov dx, 0x1f6
shr ecx, 8
mov al, 0b1111
and al, cl
mov cl, 0b1110_0000
or al, cl
out dx, al

mov dx, 0x1f7
mov al, 0x20
out dx, al
xor ecx, ecx
mov cl, bl
.read_loop:
mov dx, 0x1f7
in al, dx
and al, 0b1000_1000
cmp al, 0b0000_1000
jnz .read_wait
push cx
call .read_sector
pop cx
loop .read_loop
ret
.read_sector:
mov cx, 256
.read_once:
jmp $+2
jmp $+2
jmp $+2
mov dx, 0x1f0
in ax, dx
mov [edi], ax
add edi, 2
loop .read_once
ret
.read_wait:
jmp $+2
jmp $+2
jmp $+2
jmp .read_loop

      
    

;ah:0x0e
;al:'a'
;int 10
;打印'a'
print:
mov ah, 0x0e
.print_loop:
mov al, [si]
cmp al, 0
jz .print_end  
int 0x10
inc si
jmp .print_loop
.print_end:
ret

str_mbr:
db "mbr",10, 13, 0 ;10代表/n光标原地下一行,13代表/r复位,光标回到行首

    

;将第一块扇区填满，并保证最后俩字节为魔数，bios以此识别mbr
times 510-($-$$) db 0
db 0x55,0xaa
