#include <speaker.h>
#include <clock.h>
#include <io.h>
#include <types.h>

static uint32 volatile beeping = 0;
static bool volatile is_beeping = false;

void speaker_init()
{
    // 配置计数器 2 蜂鸣器
    outb(PIT_CTRL_REG, 0b10110110);
    outb(PIT_CHAN2_REG, (uint8)BEEP_COUNTER);
    outb(PIT_CHAN2_REG, (uint8)(BEEP_COUNTER >> 8));
}

void start_beep()
{
    if(!is_beeping)
    {
        is_beeping = true;
        outb(SPEAKER_REG, inb(SPEAKER_REG) | 0b11);
    }
}

void beeping_check()
{
    if(is_beeping)
    {
        beeping++;
        if(beeping >= 5)
        {
            beeping = 0;
            is_beeping = false;
            outb(SPEAKER_REG, inb(SPEAKER_REG) & 0xfc);
        }
    }
}