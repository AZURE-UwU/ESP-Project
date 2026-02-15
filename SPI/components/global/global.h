#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <stdint.h>
#include "function.h"





extern volatile uint64_t sys;


extern volatile uint8_t lock[MAX_KEYS];
extern volatile uint64_t btn_last_irq[MAX_KEYS];
extern volatile uint8_t longFlag[MAX_KEYS];

extern volatile uint8_t test;

#endif
