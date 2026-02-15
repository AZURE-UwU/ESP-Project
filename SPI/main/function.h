/******************************************************************************
 * @file    function.h
 * @brief   通用功能函数接口
 * @details 提供格式转换、滤波器、数值计算、按键逻辑、电源管理等常用函数声明。
 *
 * @author  Bowen
 * @date    2026-02-10
 * @version 1.1.0
 *
 * @modules
 *   - 格式转换：浮点数转字符串、时间格式化、电压转温度
 *   - 滤波器：移动平均滤波、卡尔曼滤波
 *   - 数值处理：最大值判断、符号字符串选择、PWM计算、能耗计算
 *   - 按键逻辑：按键处理、去抖动
 *   - 系统控制：电源关闭
 *
 * @note
 *   - 本头文件仅声明接口，具体实现见 function.c
 *   - 所有函数均为通用工具函数，可在不同模块中复用
 ******************************************************************************/




#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include "math.h"
#include "string.h"
#include "stdio.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif


/* 格式转换 */
void formatFloatToStr(float value, char *out, int width, int prec);
char* FormatTimeString(uint64_t msTicks);


/* 滤波器 */
#define WINDOW_SIZE 20
typedef struct {
    float buffer[WINDOW_SIZE];  // 环形缓冲区
    float sum;                  // 累加和
    uint8_t write_idx;          // 写入位置
    uint8_t count;              // 已填充数量
} MovingAverageFilter;
void MovingAverageInit(MovingAverageFilter *f);
float MovingAverage(uint8_t ch, float value);


typedef struct {
    float x_est;       // 当前估计值
    float p_est;       // 当前协方差
    uint8_t initialized; // 是否初始化过
} KalmanFilter;
void KalmanInit(KalmanFilter *kf, float init_x, float init_p);
float KalmanUpdate(KalmanFilter *kf, float value, float Q, float R);


/* 插值校准 */
#define MAX_CAL_POINTS 10
typedef struct {
    int id;
    int num_points;
    float points[MAX_CAL_POINTS];
    float offsets[MAX_CAL_POINTS];
} CalTable;
float CalibrateCurrent(const CalTable *ct, float rawCurrent);


/* 数值处理 */
float MaxValue(uint8_t id, float value);
const char* check_sign_str(float x, const char* neg_str, const char* pos_str);
uint8_t CalculatePWM(float temperature);
float calc_wh(uint64_t curr_ms, float power_w);
float Voltage_To_Temperature(float voltage);


/* SYSTEM */
#define MAX_KEYS 2
uint8_t btnLogic(uint8_t num);
uint8_t btnLogic_withExtLongFlag(uint8_t num);
void Debounce_Process(void);

void power_off(void);




#endif
