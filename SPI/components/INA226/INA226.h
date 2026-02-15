#ifndef __INA226_H
#define __INA226_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define INA226_REG_CONFIG         0x00
#define INA226_REG_SHUNTVOLTAGE   0x01
#define INA226_REG_BUSVOLTAGE     0x02
#define INA226_REG_POWER          0x03
#define INA226_REG_CURRENT        0x04
#define INA226_REG_CALIBRATION    0x05

// Calibration & shunt resistor(本代码实现绕过了校准值的计算，无需修改)
#define INA226_CALIBRATION_VALUE  1024
//#define SHUNT_RESISTOR_VALUE      0.01f
#define SHUNT_RESISTOR_VALUE      RESISTOR
extern float RESISTOR;

void INA226_Reset(uint8_t dev_addr);
void INA226_Configuration(uint8_t dev_addr, uint8_t avgSamples, uint8_t vbusCT, uint8_t vshCT, uint8_t mode);
void INA226_Init(uint8_t dev_addr, uint8_t avgSamples, uint8_t vbusCT, uint8_t vshCT, uint8_t mode);
void INA226_SetConfig(uint8_t dev_addr, uint8_t avgSamples, uint8_t vbusCT, uint8_t vshCT, uint8_t mode);

float INA226_GetBusVoltage(uint8_t dev_addr);
float INA226_GetShuntVoltage(uint8_t dev_addr);
float INA226_GetCurrent(uint8_t dev_addr);
float INA226_GetPower(uint8_t dev_addr);


#endif // __INA226_H
