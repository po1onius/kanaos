#include <console.h>
#include <io.h>
#include <device.h>


#define CRT_START_ADDR_H 0xC // 显示内存起始位置 - 高位
#define CRT_START_ADDR_L 0xD // 显示内存起始位置 - 低位


#define MEM_BASE (0xB8000 + 0xC0000000)             // 显卡内存起始位置
#define MEM_SIZE 0x4000               // 显卡内存大小
#define MEM_END (MEM_BASE + MEM_SIZE) // 显卡内存结束位置
#define WIDTH 80                      // 屏幕文本列数
#define HEIGHT 25                     // 屏幕文本行数
#define ROW_SIZE (WIDTH * 2)          // 每行字节数,两个字节显示一个字符,其中一个字节为字符,另一个为属性(色彩)
#define SCR_SIZE (ROW_SIZE * HEIGHT)  // 屏幕字节数

#define ASCII_NUL 0x00
#define ASCII_ENQ 0x05
#define ASCII_BEL 0x07 // \a
#define ASCII_BS 0x08  // \b
#define ASCII_HT 0x09  // \t
#define ASCII_LF 0x0A  // \n
#define ASCII_VT 0x0B  // \v
#define ASCII_FF 0x0C  // \f
#define ASCII_CR 0x0D  // \r
#define ASCII_DEL 0x7F


//光标当前所在
static uint32 cur_ptr;
static uint16 erase = 0x720;


static void set_cur()
{
    outb(CRT_ADDR_REG, CRT_CURSOR_H);
    outb(CRT_DATA_REG, (cur_ptr - MEM_BASE) >> 9);//每个字符占两个字节,先 >>1 再 >>8
    outb(CRT_ADDR_REG, CRT_CURSOR_L);
    outb(CRT_DATA_REG, (cur_ptr - MEM_BASE) >> 1);
}

static void scroll_line()
{
    for(uint32 i = MEM_BASE; i < cur_ptr; i += 2)
    {
        *(int8*)i = *(int8*)(i + ROW_SIZE);
    }
    cur_ptr -= ROW_SIZE;
    set_cur();
}

void console_init()
{
    //获取当前光标位置
    outb(CRT_ADDR_REG, CRT_CURSOR_H);
    uint16 cursor = inb(CRT_DATA_REG) << 8;
    outb(CRT_ADDR_REG, CRT_CURSOR_L);
    cursor |= inb(CRT_DATA_REG);

    //光标位置地址
    cur_ptr = cursor * 2 + MEM_BASE;
    console_clear();
    device_install(DEV_CHAR, DEV_CONSOLE, NULL, "console", 0, NULL, NULL, console_write);
}


void console_clear()
{
    for(uint32 i = MEM_BASE; i < cur_ptr; i += 2)
    {
        *((int8*)i) = erase;
    }
    cur_ptr = MEM_BASE;
    set_cur();
}

void console_write(void* dev, int8* str, size_t size)
{
    while(size--)
    {
        int8 ch = *(str++);
        if(ch == '\n')
        {
            cur_ptr = MEM_BASE + ROW_SIZE * ((cur_ptr - MEM_BASE) / ROW_SIZE + 1);
            set_cur();
        }
        else
        {
            *(int8*)cur_ptr = ch;
            cur_ptr += 2;
        }
        if(cur_ptr > MEM_END)
        {
            scroll_line();
        }
    }
}