[org 0x1000]
mov si, str_loader
call print


  ;bios 0x15号中断
  ;eax=0xe82 子功能 内存检测
  ;参数:
  ;ebx 初始设为0,bios会使用这个寄存器
  ;es:di 检测到的内存信息保存在该位置
  ;ecx 内存信息数据结构大小，一般为20字节
  ;edx 签名 固定为0x534d4150 ascii为“SMAP"
  ;返回值:
  ;CF CF为1表示出错
  ;eax 返回时会将eax置为0x534d4150
  ;es:di 返回型参数
  ;ecx 同参数
  ;ebx 不同内存块检测结束会改变这个值,该值为0时表示检测结束
  ;注:内存是分块检测的，每检测一块就调用一次

  ;ards内存信息结构 20字节
  ;0~3 基地址低32位
  ;4~7 基地址高32位
  ;8~11 内存长度的低32位 单位为字节
  ;12~15 内存长度的高32位 单位为字节
  ;16~19 该内存块的type. type=1内存块可用,type=2内存块不可用,type=...
detect_memroy:
xor ebx, ebx
mov ax, 0
mov es, ax
mov di, ards_buffer
mov ecx, 20
mov edx, 0x534d4150
.detect_next:
mov eax, 0xe820 ;中断调用里使用了eax，所以每次检测一块都要重新设置eax
int 0x15
jc .detect_error
inc word [ards_count]
add di, cx
cmp ebx, 0
jz .detect_end
jmp .detect_next


.detect_error:
mov si, str_error
call print
hlt
jmp $


.detect_end:
mov cx, [ards_count]
mov si, 0
.detect_debug_show:
mov eax, [ards_buffer + si] ;基地址
mov ebx, [ards_buffer + si + 8] ;长度
mov edx, [ards_buffer + si + 16] ;type
add si, 20
loop .detect_debug_show
mov si, str_detect
call print




;准备进入保护模式
;关闭中断
cli

;打开A20总线
in al, 0x92
or al, 0b10
out 0x92, al

;设置gdtr寄存器
lgdt [gdt_ptr]

;开启保护模式
mov eax, cr0
or eax, 1
mov cr0, eax




;刷新流水线,进入保护模式
jmp dword code_selector:protect


[bits 32]
protect:
mov ax, data_selector
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
mov esp, 0x10000


;从disk加载kernel
mov edi, 0x10000 
mov ecx, 10 
mov bl, 200
call read_disk

call setup_page

add esp, 0xc0000000

mov eax, PAGE_DIR_TABLE_POS
mov cr3, eax

mov eax, cr0
or eax, 0x80000000
mov cr0, eax

mov eax, 0x8259a
mov ebx, ards_count

jmp dword code_selector:0xc0010000



PG_P  equ   1b
PG_RW_R	 equ  00b 
PG_RW_W	 equ  10b 
PG_US_S	 equ  000b 
PG_US_U	 equ  100b 
PAGE_DIR_TABLE_POS equ 0x100000
;-------------   创建页目录及页表   ---------------
setup_page:
;先把页目录占用的空间逐字节清0
   mov ecx, 4096
   mov esi, 0
.clear_page_dir:
   mov byte [PAGE_DIR_TABLE_POS + esi], 0
   inc esi
   loop .clear_page_dir

;开始创建页目录项(PDE)
.create_pde:
   mov eax, PAGE_DIR_TABLE_POS
   add eax, 0x1000
   mov ebx, eax


   or eax, PG_US_U | PG_RW_W | PG_P
   mov [PAGE_DIR_TABLE_POS + 0x0], eax
   mov [PAGE_DIR_TABLE_POS + 0xc00], eax
   sub eax, 0x1000
   mov [PAGE_DIR_TABLE_POS + 4092], eax

;下面创建页表项(PTE)
   mov ecx, 256
   mov esi, 0
   mov edx, PG_US_U | PG_RW_W | PG_P
.create_pte:
   mov [ebx+esi*4],edx
   add edx,4096
   inc esi
   loop .create_pte

;创建内核其它页表的PDE
   mov eax, PAGE_DIR_TABLE_POS
   add eax, 0x2000
   or eax, PG_US_U | PG_RW_W | PG_P
   mov ebx, PAGE_DIR_TABLE_POS
   mov ecx, 254
   mov esi, 769
.create_kernel_pde:
   mov [ebx+esi*4], eax
   inc esi
   add eax, 0x1000
   loop .create_kernel_pde
   ret

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







memory_base equ 0
memory_limit equ 1024 * 1024 - 1 ;4G / 4K,粒度G为1表示4K × limit = length 

code_selector equ (1 << 3) ;segment code index 1,后三位RPL和TI都为0,移位3位即可
data_selector equ (2 << 3) ;index 2


gdt_ptr:
dw (gdt_end - gdt) - 1
dd gdt


gdt:
;第一项为0
dd 0, 0

;第二项kernel code
gdt_kernel_code:
dw memory_limit & 0xffff
dw memory_base & 0xffff
db (memory_base & 0xff0000) >> 16
db 0b_1_00_1_1_0_1_0
db 0b_1_1_0_0_0000 | ((memory_limit & 0xf0000) >> 16)
db (memory_base & 0xff000000) >> 24

;第三项kernel data
gdt_kernel_data:
dw memory_limit & 0xffff
dw memory_base & 0xffff
db (memory_base & 0xff0000) >> 16
db 0b_1_00_1_0_0_1_0
db 0b_1_1_0_0_0000 | ((memory_limit & 0xf0000) >> 16)
db (memory_base & 0xff000000) >> 24

gdt_end:



;这里还是实模式下的bios中断打印
;上面[bits 32]会影响此处代码
;此处改回[bits 16]
[bits 16]
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

str_loader:
db "loader",10, 13, 0
str_error:
db "error",10, 13, 0
str_detect:
db "detect memory success", 10, 13, 0




ards_count:
  dw 0
ards_buffer: ;内存块信息的”数组“,内存块数量不定,因此该地址放在最后以容纳变长
