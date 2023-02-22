#ifndef __IO_H__
#define __IO_H__



#define CRT_ADDR_REG 0x3d4
#define CRT_DATA_REG 0x3d5
#define CRT_CURSOR_H 0xe
#define CRT_CURSOR_L 0xf


#include <types.h>


extern uint8 inb(uint16 port);
extern uint16 inw(uint16 port);
extern void outb(uint16 port, uint8 value);
extern void outw(uint16 port, uint16 value);


#endif