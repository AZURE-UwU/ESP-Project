/******************************************************************************
 * @file    function.c
 * @brief   通用功能函数实现
 * @details 包含格式转换、滤波器、数值计算、按键逻辑、电源管理等函数的具体实现。
 *
 * @author  Bowen
 * @date    2026-02-10
 * @version 1.2.0
 *
 * @hardware
 *   MCU:      STM32G030F6P6
 *   Display:  ST7789 320x172 SPI
 *
 * @note
 *   - 本文件为 function.h 中声明的函数的实现部分
 *   - 所有函数均为通用工具函数，可在不同模块中复用
 ******************************************************************************/
/******************************************************************************
 * @license
 *   Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *   (CC BY-NC-SA 4.0)
 *
 *   本源码仅供学习与个人使用，禁止任何商业用途。
 *   使用者必须：
 *     - 署名 (BY)：在使用或修改本代码时注明原作者 Bowen。
 *     - 非商业 (NC)：不得将本代码或其修改版本用于商业目的。
 *     - 相同方式共享 (SA)：若修改或衍生本代码，必须以相同的 CC BY-NC-SA 协议发布。
 *
 *   本代码按“现状”提供，不附带任何保证。作者不对使用过程中产生的任何损失负责。
 *
 *   完整协议文本请参阅：
 *   https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode
 *
 * @note
 *   作者本人保留商业使用权，并可另行授权他人商业使用。
 *   若您已获得作者的商业授权，请遵循商业授权协议的条款，
 *   而非 CC BY-NC-SA 的限制。
 *
 *   商业授权特别说明：
 *     - 商业授权用户可以在商业项目中使用本源码。
 *     - 商业授权用户不得再次分发本源码或其修改版本，
 *       包括但不限于公开发布、转售或再授权。
 *     - 商业授权仅限于获得许可的个人或组织，不得转移。
 ******************************************************************************/

/* ============================================================================
   CHANGELOG
   ----------------------------------------------------------------------------
   2026-01-29  v1.0.0  Bowen  - 初始版本，包含基本工具函数
   2026-02-09  v1.0.1  Bowen  - 增加中断回调函数的两种写法，适配 M0+ 内核睡眠模式
   2026-02-10  v1.1.0  Bowen  - 增加新的滤波算法，修改滤波器调用逻辑，整理头文件
   2026-02-11  v1.2.0  Bowen  - 增加新的按钮逻辑，新增双击逻辑，新增可连续触发长按逻辑
   ========================================================================== */

#include "function.h"
#include "main.h"
#include "global.h"
#include "st7789.h"

/* ------------------------------------------------------------------
  格式化变量
   ------------------------------------------------------------------ */
void formatFloatToStr(float value, char *out, int width, int prec)
{
    if (out == NULL || width <= 0) return;

    /* NaN 或 Inf 处理 */
    if (!isfinite(value)) {
        memset(out, 'x', width);
        out[width] = '\0';
        return;
    }

    /* 处理符号与绝对值 */
    int negative = signbit(value) ? 1 : 0;
    float absval = negative ? -value : value;

    /* 取整数部分（截断） */
    char intbuf[64];
    long long intpart = (long long)absval; /* 截断 */
    snprintf(intbuf, sizeof(intbuf), "%lld", (long long)intpart);
    int intlen = (int)strlen(intbuf);

    int sign_len = negative ? 1 : 0;

    /* 如果整数部分（含符号）已超出宽度，则无法显示 */
    if (sign_len + intlen > width) {
        memset(out, 'x', width);
        out[width] = '\0';
        return;
    }

    /* 计算最多能放的小数位数（不超过用户给定的 prec） */
    int max_frac = 0;
    if (prec > 0) {
        int avail = width - (sign_len + intlen + 1); /* 预留小数点 */
        if (avail > 0) max_frac = avail;
        else max_frac = 0;
        if (max_frac > prec) max_frac = prec;
    } else {
        max_frac = 0;
    }

    /* 计算实际需要的字符数（含小数点和小数位，如果有） */
    int needed = sign_len + intlen + (max_frac > 0 ? (1 + max_frac) : 0);

    /* 只要宽度大于需要的字符数，就在整数左侧（符号之后）填充 '0' */
    int pad_zeros = 0;
    if (width > needed) {
        pad_zeros = width - needed;
    }

    /* 构造输出：符号 -> 填充0（若有） -> 整数部分 -> 小数点 -> 小数部分 */
    char tmp[128];
    int pos = 0;

    if (negative) {
        tmp[pos++] = '-';
    }

    for (int i = 0; i < pad_zeros; ++i) {
        tmp[pos++] = '0';
    }

    memcpy(tmp + pos, intbuf, intlen);
    pos += intlen;

    if (max_frac > 0) {
        tmp[pos++] = '.';
        float frac = absval - (float)intpart;
        if (frac < 0.0f) frac = 0.0f;
        for (int i = 0; i < max_frac; ++i) {
            frac *= 10.0f;
            int digit = (int)frac;
            if (digit < 0) digit = 0;
            if (digit > 9) digit = 9;
            tmp[pos++] = (char)('0' + digit);
            frac -= (float)digit;
        }
    }

    int outlen = pos;
    if (outlen > width) {
        memset(out, '.', width);
        out[width] = '\0';
        return;
    }

    memcpy(out, tmp, outlen);
    out[outlen] = '\0';
}


/* ------------------------------------------------------------------
  时间生成器
   ------------------------------------------------------------------ */
char* FormatTimeString(uint64_t sys)
{
    static char buf[20];
    uint32_t total_sec  = sys / 1000;
    uint32_t total_hour = total_sec / 3600;

    if (total_hour <= 99) {
        uint32_t hh = total_hour;
        uint32_t mm = (total_sec % 3600) / 60;
        uint32_t ss = total_sec % 60;
        snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hh, mm, ss);
    } else {
        uint32_t days = total_sec / 86400;
        uint32_t rem  = total_sec % 86400;
        uint32_t hh   = rem / 3600;
        uint32_t mm   = (rem % 3600) / 60;
        snprintf(buf, sizeof(buf), "%ud%02uh%02um", days, hh, mm);
    }

    return buf;
}




/* ------------------------------------------------------------------
  多通道滑动均值
   ------------------------------------------------------------------ */
/* 初始化滤波器 */
void MovingAverageInit(MovingAverageFilter *f) {
    f->sum = 0.0f;
    f->write_idx = 0;
    f->count = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        f->buffer[i] = 0.0f;
    }
}

/* 更新滤波器并返回平均值 */
float MovingAverageUpdate(MovingAverageFilter *f, float value) {
    float old_value = f->buffer[f->write_idx];

    // 写入新值
    f->buffer[f->write_idx] = value;
    f->sum += value - old_value;

    // 移动写入指针
    f->write_idx = (f->write_idx + 1) % WINDOW_SIZE;

    // 更新计数
    if (f->count < WINDOW_SIZE) {
        f->count++;
    }

    return f->sum / f->count;
}

/* ------------------------------------------------------------------
  卡尔曼滤波
   ------------------------------------------------------------------ */

/* 初始化滤波器 */
void KalmanInit(KalmanFilter *kf, float init_x, float init_p) {
    kf->x_est = init_x;
    kf->p_est = init_p;
    kf->initialized = 1;
}

/* 卡尔曼滤波更新 */
float KalmanUpdate(KalmanFilter *kf, float value, float Q, float R) {
    if (!kf->initialized) {
        kf->x_est = value;
        kf->p_est = 1.0f;
        kf->initialized = 1;
        return value;
    }

    // 预测
    float x_pred = kf->x_est;
    float p_pred = kf->p_est + Q;

    // 更新
    float K = p_pred / (p_pred + R);
    kf->x_est = x_pred + K * (value - x_pred);
    kf->p_est = (1.0f - K) * p_pred;

    return kf->x_est;
}



/* ------------------------------------------------------------------
  插值校准
   ------------------------------------------------------------------ */


float CalibrateCurrent(const CalTable *ct, float rawCurrent) {
    const float *points = ct->points;
    const float *offs   = ct->offsets;
    int n = ct->num_points;

    // 低于最小点
    if (rawCurrent <= points[0]) {
        return rawCurrent + offs[0];
    }
    // 高于最大点
    if (rawCurrent >= points[n - 1]) {
        return rawCurrent + offs[n - 1];
    }

    // 分段线性插值
    for (int i = 0; i < n - 1; i++) {
        float x0 = points[i];
        float x1 = points[i + 1];
        if (rawCurrent > x0 && rawCurrent <= x1) { // 边界归属右区间
            float o0 = offs[i];
            float o1 = offs[i + 1];
            float slope = (o1 - o0) / (x1 - x0);
            float o = o0 + slope * (rawCurrent - x0);
            return rawCurrent + o;
        }
    }

    return rawCurrent; // 理论上不会到达
}


/* ------------------------------------------------------------------
  多通道最大值查找
   ------------------------------------------------------------------ */
#define MAX_CHANNELS_MAX  8
static float max_values[MAX_CHANNELS_MAX];
/* 每路通道是否已初始化过最大值 */
static uint8_t max_inited[MAX_CHANNELS_MAX] = {0};
float MaxValue(uint8_t id, float value)
{

    /* 边界检查 */
    if (id >= MAX_CHANNELS_MAX) {
        /* 超出通道范围，直接返回输入值 */
        return value;
    }

    /* 首次调用：初始化该通道最大值 */
    if (!max_inited[id]) {
        max_values[id]   = value;
        max_inited[id]   = 1;
    }
    else if (value > max_values[id]) {
        /* 更新更大的值 */
        max_values[id] = value;
    }

    return max_values[id];
}

/* ------------------------------------------------------------------
  检测数据正负并返回指定字符
   ------------------------------------------------------------------ */
const char* check_sign_str(float x, const char* neg_str, const char* pos_str)
{
    union {
        float    f;
        unsigned u;
    } un = { x };

    return (un.u & 0x80000000U) ? neg_str : pos_str;
}


/* ------------------------------------------------------------------
  温度转换为百分比
   ------------------------------------------------------------------ */

//—— 温度阈值（单位：°C） —— 
#define TEMP_OFF        48.0f   // OFF → SPAN 的临界温度
#define TEMP_SPAN       55.0f   // SPAN → ON 的临界温度
#define TEMP_SET        70.0f   // ON → FULL 的临界温度

//—— 各区间占空比配置（单位：%） —— 
#define PWM_OFF         0      // OFF 区固定占空比
#define PWM_SPAN_DOWN   10    // 降温进入 SPAN 时的占空比
#define PWM_ON          PWM_SPAN_DOWN      // ON 区线性起点占空比
#define PWM_MAX         99     // 满占空比

// 状态枚举：OFF、SPAN、ON
typedef enum {
    STATE_OFF = 0,
    STATE_SPAN,
    STATE_ON
} PWM_State;
uint8_t CalculatePWM(float temperature)
{
	/**
 * 按照 OFF→SPAN→ON→FULL→SPAN→OFF 的入口方向输出占空比：
 * - OFF 区：t < TEMP_OFF，输出 PWM_OFF
 * - SPAN 区：TEMP_OFF ≤ t < TEMP_SPAN
 *      ? 如果上一次非 SPAN 是 OFF → 输出 PWM_OFF
 *      ? 如果上一次非 SPAN 是 ON  → 输出 PWM_SPAN_DOWN
 * - ON 区：t ≥ TEMP_SPAN
 *      ? TEMP_SPAN ≤ t < TEMP_SET：线性 PWM_ON→PWM_MAX
 *      ? t ≥ TEMP_SET：输出 PWM_MAX
 */
    // 记忆上一次的非 SPAN 状态（初始假定为 OFF）
    static PWM_State lastNonSpan = STATE_OFF;

    // 判定当前区间
    PWM_State curState;
    if (temperature < TEMP_OFF) {
        curState = STATE_OFF;
    }
    else if (temperature < TEMP_SPAN) {
        curState = STATE_SPAN;
    }
    else {
        curState = STATE_ON;
    }

    // 进入非 SPAN 时，更新 lastNonSpan
    if (curState != STATE_SPAN) {
        lastNonSpan = curState;
    }

    // 计算 PWM
    uint8_t pwm;
    switch (curState) {
        case STATE_OFF:
            pwm = PWM_OFF;
            break;

        case STATE_SPAN:
            // 从 OFF→SPAN 或从 ON→SPAN 的区分
            pwm = (lastNonSpan == STATE_OFF) ? PWM_OFF : PWM_SPAN_DOWN;
            break;

        case STATE_ON:
        default:
            // ON、FULL 区：先做线性，再满速
            if (temperature < TEMP_SET) {
                float ratio = (temperature - TEMP_SPAN) / (TEMP_SET - TEMP_SPAN);
                pwm = PWM_ON + (uint8_t)((PWM_MAX - PWM_ON) * ratio + 0.5f);
            }
            else {
                pwm = PWM_MAX;
            }
            break;
    }

    return pwm;
}



/* ------------------------------------------------------------------
  计算瓦时
   ------------------------------------------------------------------ */
float calc_wh(uint64_t curr_ms, float power_w) {
    static uint64_t prev_ms = 0;   // 保存上一次时间戳
    static float total_wh = 0.0f;  // 累计能量 (Wh)

    if (prev_ms == 0) {
        // 第一次调用，只记录时间戳，不计算
        prev_ms = curr_ms;
        return total_wh;
    }

    // 时间差 (毫秒)
    uint64_t delta_ms = curr_ms - prev_ms;
    prev_ms = curr_ms;

    // 转换为小时
    float delta_h = (float)delta_ms / 3600000.0f;

    // 累加能量
    total_wh += power_w * delta_h;

    return total_wh;
}



/* ------------------------------------------------------------------
  电压值转换为温度
   ------------------------------------------------------------------ */
#define VCC         3.3f
#define B_VALUE     3950.0f
#define R0          10000.0f
#define T0_KELVIN   298.15f

// 选择方案：1=上拉电阻，2=下拉电阻
#define NTC_SCHEME  1

#if (NTC_SCHEME == 1)
// 方案1: 上拉电阻
#define R_PULLUP    10000.0f
float Voltage_To_Temperature(float voltage)
{
    if (voltage <= 0.0f || voltage >= VCC)
        return -273.15f;  // 异常情况

    float r_ntc = R_PULLUP * voltage / (VCC - voltage);
    float inv_T = 1.0f / T0_KELVIN + logf(r_ntc / R0) / B_VALUE;
    float temp_k = 1.0f / inv_T;
    return temp_k - 273.15f;
}

#elif (NTC_SCHEME == 2)
// 方案2: 下拉电阻
#define R_PULLDOWN  10000.0f
float Voltage_To_Temperature(float voltage)
{
    if (voltage <= 0.0f)
        return -273.15f;  // 异常情况

    float r_ntc = R_PULLDOWN * (VCC - voltage) / voltage;
    float inv_T = 1.0f / T0_KELVIN + logf(r_ntc / R0) / B_VALUE;
    float temp_k = 1.0f / inv_T;
    return temp_k - 273.15f;
}
#endif

/* ------------------------------------------------------------------
  多按钮逻辑判断
   ------------------------------------------------------------------ */
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//{
//    uint8_t idx;
//    switch(GPIO_Pin)
//    {
//        case SW_WKUP_Pin: idx = 0; break;
//        case SW_FUNC_Pin: idx = 1; break;
//        default: return;
//    }

//    btn_last_irq[idx] = sys;
//}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    uint8_t idx;
    switch(GPIO_Pin)
    {
        case SW_WKUP_Pin: idx = 0; break;
        case SW_FUNC_Pin: idx = 1; break;
        default: return;
    }

    btn_last_irq[idx] = sys;
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    uint8_t idx;
    switch(GPIO_Pin)
    {
        case SW_WKUP_Pin: idx = 0; break;
        case SW_FUNC_Pin: idx = 1; break;
        default: return;
    }

    btn_last_irq[idx] = sys;
}



#define DEBOUNCE_TIME 10
volatile uint64_t btn_press_start[MAX_KEYS] = {0};
volatile uint64_t btn_press_duration[MAX_KEYS] = {0};
volatile uint8_t btn_flag[MAX_KEYS] = {0};

// 在主循环或定时器中周期性调用此函数
void Debounce_Process(void)
{
		static volatile uint8_t  btn_stable_state[MAX_KEYS] = {0};
		
    for (uint8_t i = 0; i < MAX_KEYS; i++) {
        if (btn_last_irq[i] == 0) continue;  // 没有触发

        if ((sys - btn_last_irq[i]) >= DEBOUNCE_TIME) {
            // 已经过了消抖时间，读取稳定电平
            uint8_t cur_level;
            if (i == 0) cur_level = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) ? 1 : 0;
            else if (i == 1) cur_level = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_SET) ? 1 : 0;

            // 状态变化才更新对外变量
            if (cur_level != btn_stable_state[i]) {
                if (cur_level == 1) {
                    // 按下
                    btn_press_start[i] = sys;
                    btn_flag[i] = 1;
                } else {
                    // 松开
                    btn_press_duration[i] = sys - btn_press_start[i];
                    btn_flag[i] = 0;
                }
                btn_stable_state[i] = cur_level;
            }

            // 清零触发时间，避免重复处理
            btn_last_irq[i] = 0;
        }
    }
}


/*
按钮逻辑中：
	访问一次就会更新一次按钮状态
	会读取全局变量：按钮是否按下，按钮按下时间，系统时间
	一旦松开或者超时，就会返回对应值 并 上锁，无法再次触发


注意：调用该函数后应当立即调用判断函数，它仅会返回一次变化值，之后返回空值

*/



#define LONG_PRESS_THRESHOLD   1000   // 长按阈值，单位 ms
#define DOUBLE_CLICK_THRESHOLD 350   // 双击最大间隔，单位 ms

// 返回值定义
#define BTN_NONE   0
#define BTN_LONG   1
#define BTN_SHORT  2
#define BTN_DOUBLE 3

//每次只能触发一次长按

//不带双击版本
//uint8_t btnLogic(uint8_t num)
//{
//    // 参数检查（假设最多3个按钮）
//    if (num >= MAX_KEYS) return BTN_NONE;

//    // 静态保存上次返回值和锁
//    static uint8_t lastValue[MAX_KEYS] = {BTN_NONE};


//    // 读取共享变量到本地，减少并发问题
//		__disable_irq();
//    uint8_t  btEN 			= 	btn_flag[num]; 
//		uint64_t sys_inline = 	sys; 
//		uint64_t btn_start 	= 	btn_press_start[num]; 
//		uint64_t duration 	=  	sys_inline - btn_start;
//		__enable_irq();
//		

//    // 如果已经触发过，返回
//    if (lock[num]) {
//			return BTN_NONE;
//    }

//    // 1) 长按判定：按下且超过阈值 -> 立即触发长按并加锁
//    if (btEN && duration >= LONG_PRESS_THRESHOLD && !longFlag[num]) {
//      lock[num] = 1;
//      lastValue[num] = BTN_LONG;
//			longFlag[num] = 1;//松开前不允许再次触发
//      return lastValue[num];
//    }

//    // 2) 短按判定：按键已松开且有记录的按下时长
//    //    这里 btn_press_duration[num] 在按键松开时被中断设置为实际时长（非0）
//    if (!btEN && btn_press_duration[num] > 0) {
//        // 如果持续时间小于长按阈值，判定为短按
//        if ((uint64_t)btn_press_duration[num] < LONG_PRESS_THRESHOLD) {
//            lock[num] = 1;
//            lastValue[num] = BTN_SHORT;
//            // 清除持续时间，避免重复判定
//            btn_press_duration[num] = 0;
//            return lastValue[num];
//        } else {
////            lock[num] = 1;
////            lastValue[num] = BTN_LONG;
////            return lastValue[num];
//					//长按按钮松开
//					btn_press_duration[num] = 0;
//					longFlag[num] = 0;
//					return BTN_NONE;
//        }
//    }

//    // 3) 无事件，返回 NONE（不更新 lastValue）
//    return BTN_NONE;
//}




//带双击版本
#define LONG_PRESS_THRESHOLD   1000   // 长按阈值，单位 ms
#define DOUBLE_CLICK_THRESHOLD 350   // 双击最大间隔，单位 ms

// 返回值定义
#define BTN_NONE   0
#define BTN_LONG   1
#define BTN_SHORT  2
#define BTN_DOUBLE 3

uint8_t btnLogic(uint8_t num)
{
    if (num >= MAX_KEYS) return BTN_NONE;

    static uint8_t lastValue[MAX_KEYS] = {BTN_NONE};
    static uint8_t pendingShort[MAX_KEYS] = {0};
    static uint64_t lastReleaseTime[MAX_KEYS] = {0};

    __disable_irq();
    uint8_t  btEN        = btn_flag[num]; 
    uint64_t sys_inline  = sys; 
    uint64_t btn_start   = btn_press_start[num]; 
    uint64_t duration    = sys_inline - btn_start;
    __enable_irq();

    if (lock[num]) {
        return BTN_NONE;
    }

    // 1) 长按判定
    if (btEN && duration >= LONG_PRESS_THRESHOLD && !longFlag[num]) {
        lock[num] = 1;
        lastValue[num] = BTN_LONG;
        longFlag[num] = 1;
        return lastValue[num];
    }

    // 2) 短按/双击判定
    if (!btEN && btn_press_duration[num] > 0) {
        if ((uint64_t)btn_press_duration[num] < LONG_PRESS_THRESHOLD) {
            uint64_t now = sys_inline;
            if (pendingShort[num] && (now - lastReleaseTime[num] <= DOUBLE_CLICK_THRESHOLD)) {
                // 第二次短按在阈值内 -> 双击
                pendingShort[num] = 0;
                lock[num] = 1;
                lastValue[num] = BTN_DOUBLE;
                btn_press_duration[num] = 0;
                return lastValue[num];
            } else {
                // 第一次短按，先挂起
                pendingShort[num] = 1;
                lastReleaseTime[num] = now;
                btn_press_duration[num] = 0;
                return BTN_NONE; // 暂时不返回，等待第二次
            }
        } else {
            // 长按松开
            btn_press_duration[num] = 0;
            longFlag[num] = 0;
            return BTN_NONE;
        }
    }

    // 3) 检查挂起的短按是否超时
    if (pendingShort[num] && (sys_inline - lastReleaseTime[num] > DOUBLE_CLICK_THRESHOLD)) {
        pendingShort[num] = 0;
        lock[num] = 1;
        lastValue[num] = BTN_SHORT;
        return lastValue[num];
    }

    return BTN_NONE;
}


//可触发多次长按,外部锁定
uint8_t btnLogic_withExtLongFlag(uint8_t num)
{
    if (num >= MAX_KEYS) return BTN_NONE;

    static uint8_t lastValue[MAX_KEYS] = {BTN_NONE};
    static uint8_t pendingShort[MAX_KEYS] = {0};
    static uint64_t lastReleaseTime[MAX_KEYS] = {0};

    __disable_irq();
    uint8_t  btEN        = btn_flag[num]; 
    uint64_t sys_inline  = sys; 
    uint64_t btn_start   = btn_press_start[num]; 
    uint64_t duration    = sys_inline - btn_start;
    __enable_irq();

    if (lock[num]) {
        return BTN_NONE;
    }

    // 1) 长按判定
    if (btEN && duration >= LONG_PRESS_THRESHOLD && !longFlag[num]) {
        lock[num] = 1;
        lastValue[num] = BTN_LONG;
//        longFlag[num] = 1;
        return lastValue[num];
    }

    // 2) 短按/双击判定
    if (!btEN && btn_press_duration[num] > 0) {
        if ((uint64_t)btn_press_duration[num] < LONG_PRESS_THRESHOLD) {
            uint64_t now = sys_inline;
            if (pendingShort[num] && (now - lastReleaseTime[num] <= DOUBLE_CLICK_THRESHOLD)) {
                // 第二次短按在阈值内 -> 双击
                pendingShort[num] = 0;
                lock[num] = 1;
                lastValue[num] = BTN_DOUBLE;
                btn_press_duration[num] = 0;
                return lastValue[num];
            } else {
                // 第一次短按，先挂起
                pendingShort[num] = 1;
                lastReleaseTime[num] = now;
                btn_press_duration[num] = 0;
                return BTN_NONE; // 暂时不返回，等待第二次
            }
        } else {
            // 长按松开
            btn_press_duration[num] = 0;
            longFlag[num] = 0;
            return BTN_NONE;
        }
    }

    // 3) 检查挂起的短按是否超时
    if (pendingShort[num] && (sys_inline - lastReleaseTime[num] > DOUBLE_CLICK_THRESHOLD)) {
        pendingShort[num] = 0;
        lock[num] = 1;
        lastValue[num] = BTN_SHORT;
        return lastValue[num];
    }

    return BTN_NONE;
}













/* ------------------------------------------------------------------
  关机相关程序
   ------------------------------------------------------------------ */

////适用Cortex_M3
//void Enter_StandbyMode(void)
//{
//    /**
//     * @brief  进入 Standby 模式，PA0 上升沿唤醒
//     */

//    /* 1. 使能 PWR 外设时钟 */
//    __HAL_RCC_PWR_CLK_ENABLE();

//    /* 2. 清除 Wake-Up 标志，确保下次能正确唤醒 */
//    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF);
////	  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

//    /* 3. 使能 WKUP 引脚（PA0） */
//    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);

//    /* 4. 设置 Cortex-M3 进入深度睡眠 */
//    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

//    /* 5. 进入 Standby 模式，不会返回，除非由 PA0 唤醒 */
//    HAL_PWR_EnterSTANDBYMode();
//}

//void GPIO_To_AnalogInput(void)
//{
//    GPIO_InitTypeDef GPIO_InitStruct = {0};

//    __HAL_RCC_GPIOA_CLK_ENABLE();
//    __HAL_RCC_GPIOB_CLK_ENABLE();
//    __HAL_RCC_GPIOC_CLK_ENABLE();
//    __HAL_RCC_GPIOD_CLK_ENABLE();

//    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
//    GPIO_InitStruct.Pull = GPIO_NOPULL;

//    // A 端口
//    GPIO_InitStruct.Pin = GPIO_PIN_All & (~GPIO_PIN_0);
//    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

//    // B 端口
//    GPIO_InitStruct.Pin = GPIO_PIN_All;
//    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//    // C 端口
//    GPIO_InitStruct.Pin = GPIO_PIN_All;
//    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

//    // D 端口（如果有）
//    GPIO_InitStruct.Pin = GPIO_PIN_All;
//    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
//}




//适用Cortex_M0+
void Enter_StandbyMode(void)
{
    /**
     * @brief  进入 Standby 模式，PA0 上升沿唤醒
     */

    /* 1. 使能 PWR 外设时钟 */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* 2. 清除 Wake-Up 标志，确保下次能正确唤醒 */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF);

    /* 3. 使能 WKUP 引脚（PA0） */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);

    /* 4. 设置 Cortex-M0+ 进入深度睡眠 */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* 5. 进入 Standby 模式，不会返回，除非由 PA0 唤醒 */
    HAL_PWR_EnterSTANDBYMode();
}
void GPIO_To_AnalogInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    // G030 没有 GPIOD，去掉

    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    // A 端口，保留 PA0 作为 WKUP 引脚
    GPIO_InitStruct.Pin = GPIO_PIN_All & (~GPIO_PIN_0);
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // B 端口
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // C 端口
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void power_off(void)
{
	LCD_Clear(BLACK);LCD_BLK(100);
	LCD_ShowString_24_12(100,65,RED,BLACK,"POWEROFF");
	// 假设主频 72MHz，大约 6400000 次循环 ≈ 1 秒
  for (volatile uint32_t i = 0; i < 6400000; i++) {__NOP();}
	LCD_BLK(0);
	LCD_Clear(BLACK);
	GPIO_To_AnalogInput();
	Enter_StandbyMode();
}

