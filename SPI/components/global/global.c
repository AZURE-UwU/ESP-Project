#include "global.h"

//长时间系统时钟
volatile uint64_t sys = 0;

//按钮逻辑传递

volatile uint8_t lock[MAX_KEYS] = {0};//不允许再次触发
volatile uint64_t btn_last_irq[MAX_KEYS] = {0};
volatile uint8_t longFlag[MAX_KEYS]  = {0};



volatile uint8_t test = 0;
