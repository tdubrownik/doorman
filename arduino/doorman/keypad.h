#ifndef __DOORMAN_KEYPAD_H__
#define __DOORMAN_KEYPAD_H__

#include <Arduino.h>

void keypad_init(void);
boolean keypad_pin_get(unsigned long  * p_pin);
void keypad_off(void);
void keypad_pin_ok(void);
void keypad_pin_wrong(void);

#endif
