#ifndef __SPEAKER_H__
#define __SPEAKER_H__


#define SPEAKER_REG 0x61
#define BEEP_HZ 440
#define BEEP_COUNTER (OSCILLATOR / BEEP_HZ)

void speaker_init();
void start_beep();
void beeping_check();

#endif

