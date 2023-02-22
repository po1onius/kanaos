#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__
#include <types.h>


void keyboard_handler();
void keyboard_init();
uint32 keyboard_read(void* dev, int8 *buf, uint32 count);

#endif
