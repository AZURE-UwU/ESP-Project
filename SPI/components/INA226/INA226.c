#include "ina226.h"
#include "i2c.h"


/* I2C 传输超时（ms） */
#ifndef INA226_I2C_TIMEOUT_MS
#define INA226_I2C_TIMEOUT_MS  500
#endif

/* Configuration Register 字段位 */
#define INA226_CFG_RST      (1U<<15)
#define INA226_CFG_AVG(x)   (((x)&0x7)<<12)
#define INA226_CFG_VBUSCT(x) (((x)&0x7)<<9)
#define INA226_CFG_VSHCT(x)  (((x)&0x7)<<6)
#define INA226_CFG_MODE(x)   (((x)&0x7)<<0)

/*
转换时间编码表 (x=0..7): 
0=140µs		1=204µs			2=332µs			3=588µs, 
4=1.1ms		5=2.116ms		6=4.156ms		7=8.244ms

平均次数编码表 (x=0..7): 
0=1			1=4			2=16		3=64, 
4=128		5=256		6=512		7=1024

整个平均周期 = 转换时间 × 平均次数



MODE[2:0][Bin]	模式编号[Dec]		模式名称							描述
000							0								Power‐down						关闭 ADC，功耗最低，不进行任何转换
001							1								Shunt, Triggered			触发一次分流电压 (Shunt) 转换，完成后进入 Power‐down
010							2								Bus, Triggered				触发一次母线电压 (Bus) 转换，完成后进入 Power‐down
011							3								Shunt+Bus, Triggered	触发一次分流和母线电压转换，完成后进入 Power‐down
100							4								ADC‐OFF							关闭 ADC，与 Power‐down 效果相同，保留其他功能
101							5								Shunt, Continuous			持续进行分流电压转换，转换时间由 VSHCT 决定，直至切换模式或复位
110							6								Bus, Continuous				持续进行母线电压转换，转换时间由 VBUSCT 决定，直至切换模式或复位
111							7								Shunt+Bus, Continuous	持续进行分流和母线电压转换，转换时间交替由 VSHCT 与 VBUSCT 决定，直至切换模式或复位

*/


/* ------------------------------------------------------------------
  低层 I2C 读写
	 ------------------------------------------------------------------ */
void INA226_WriteRegister(uint8_t dev_addr, uint8_t reg, uint16_t value)
{
    uint8_t tx[3];
    tx[0] = reg;
    tx[1] = (uint8_t)(value >> 8);
    tx[2] = (uint8_t)(value & 0xFF);

    uint16_t devAddr8 = (uint16_t)(dev_addr << 1); /* HAL 使用 8-bit 地址 */

    HAL_I2C_Master_Transmit(&hi2c2, devAddr8, tx, sizeof(tx), INA226_I2C_TIMEOUT_MS);
}

uint16_t INA226_ReadRegister(uint8_t dev_addr, uint8_t reg)
{
    uint8_t buf[2] = {0};
    uint16_t devAddr8 = (uint16_t)(dev_addr << 1);

    HAL_I2C_Mem_Read(&hi2c2,
                     devAddr8,
                     reg,
                     I2C_MEMADD_SIZE_8BIT,
                     buf,
                     2,
                     INA226_I2C_TIMEOUT_MS);

    return ((uint16_t)buf[0] << 8) | buf[1];
}


/* ------------------------------------------------------------------
  对外API
	 ------------------------------------------------------------------ */
/* Issue a soft reset */
void INA226_Reset(uint8_t dev_addr)
{
    INA226_WriteRegister(dev_addr, INA226_REG_CONFIG, 0x8000);
}

/* Configure averaging, conversion times, mode */
void INA226_Configuration(uint8_t dev_addr, uint8_t avgSamples, uint8_t vbusCT, uint8_t vshCT, uint8_t mode)
{
    uint16_t cfg = 0;
    cfg |= INA226_CFG_AVG(avgSamples & 0x7);
    cfg |= INA226_CFG_VBUSCT(vbusCT & 0x7);
    cfg |= INA226_CFG_VSHCT(vshCT & 0x7);
    cfg |= INA226_CFG_MODE(mode & 0x7);
    INA226_WriteRegister(dev_addr, INA226_REG_CONFIG, cfg);
}

/* Initialize device: reset + config + calibration */
void INA226_Init(uint8_t dev_addr, uint8_t avgSamples, uint8_t vbusCT, uint8_t vshCT, uint8_t mode)
{
    INA226_Reset(dev_addr);
    HAL_Delay(5);
    INA226_Configuration(dev_addr, avgSamples, vbusCT, vshCT, mode);
    INA226_WriteRegister(dev_addr, INA226_REG_CALIBRATION, INA226_CALIBRATION_VALUE);
}

/**
 * @brief 更新 INA226 配置：滤波（平均）、转换时间与模式（单独接口）
 * @param dev_addr 7-bit 地址
 * @param avgSamples 平均次数编码（0..7）
 * @param vbusCT 母线转换时间编码（0..7）
 * @param vshCT 分流转换时间编码（0..7）
 * @param mode 工作模式编码（0..7）
 */
void INA226_SetConfig(uint8_t dev_addr,
                        uint8_t avgSamples,
                        uint8_t vbusCT,
                        uint8_t vshCT,
                        uint8_t mode)
{
    INA226_Configuration(dev_addr, avgSamples, vbusCT, vshCT, mode);
}

/* Convert raw to bus voltage (V) */
float INA226_GetBusVoltage(uint8_t dev_addr)
{
    uint16_t raw = INA226_ReadRegister(dev_addr, INA226_REG_BUSVOLTAGE);
    return (float)raw * 1.25f / 1000.0f;  /* 1 LSB = 1.25 mV */
}

/* Shunt voltage (signed) in volts */
float INA226_GetShuntVoltage(uint8_t dev_addr)
{
    int16_t raw = (int16_t)INA226_ReadRegister(dev_addr, INA226_REG_SHUNTVOLTAGE);
    return (float)raw * 2.5f / 1000.0f;  /* 1 LSB = 2.5 µV */
}

/* Compute current using calibration LSB (A) */
float INA226_GetCurrent(uint8_t dev_addr)
{
    int16_t raw = (int16_t)INA226_ReadRegister(dev_addr, INA226_REG_CURRENT);
    float current_lsb = 0.00512f / (INA226_CALIBRATION_VALUE * SHUNT_RESISTOR_VALUE);
    return (float)raw * current_lsb;
}

/* Compute power using built-in power LSB = 25 × current LSB (W) */
float INA226_GetPower(uint8_t dev_addr)
{
    uint16_t raw = INA226_ReadRegister(dev_addr, INA226_REG_POWER);
    float current_lsb = 0.00512f / (INA226_CALIBRATION_VALUE * SHUNT_RESISTOR_VALUE);
    float power_lsb   = current_lsb * 25.0f;
    return (float)raw * power_lsb;
}
