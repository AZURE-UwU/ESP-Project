/******************************************************************************
 * @file    st7789.h
 * @brief   ST7789 LCD 驱动接口
 * @details 提供 LCD 初始化、像素/线/圆等绘图原语以及配置选项。
 *
 * @author  Bowen
 * @date    2026-02-08
 * @version 1.1.1
 *
 * @hardware
 *   MCU:      STM32F103C8T6
 *   Display:  ST7789 320x172 SPI
 *
 * @note
 *   - 本头文件仅声明接口，具体实现见 lcd_driver.c
 *   - 所有坐标均以左上角为 (0,0)，x 向右，y 向下
 ******************************************************************************/


#ifndef __ST7789_H__
#define __ST7789_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "main.h"


#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------
   硬件引脚定义
   DC  -> PA6
   CS  -> PA4
   RES -> PA5
   BLK -> PA7
   ------------------------------------------------------------------ */

/* DC - PA6 */
#define DC_PIN               LL_GPIO_PIN_6
#define DC_H()               LL_GPIO_SetOutputPin(GPIOA, DC_PIN)
#define DC_L()               LL_GPIO_ResetOutputPin(GPIOA, DC_PIN)

/* CS - PA4 */
#define CS_PIN               LL_GPIO_PIN_4
#define CS_H()               LL_GPIO_SetOutputPin(GPIOA, CS_PIN)
#define CS_L()               LL_GPIO_ResetOutputPin(GPIOA, CS_PIN)

/* RES - PA5 */
#define RST_PIN              LL_GPIO_PIN_5
#define RST_H()              LL_GPIO_SetOutputPin(GPIOA, RST_PIN)
#define RST_L()              LL_GPIO_ResetOutputPin(GPIOA, RST_PIN)

/* BLK - PA7 (背光) */
#define BLK_PIN              LL_GPIO_PIN_7
#define BLK_H()              LL_GPIO_SetOutputPin(GPIOA, BLK_PIN)
#define BLK_L()              LL_GPIO_ResetOutputPin(GPIOA, BLK_PIN)

/* SPI 选择（默认使用 SPI1） */
#ifndef ST7789_SPI
#define ST7789_SPI           SPI1
#endif

/* 屏幕方向与尺寸 */
#ifndef USE_HORIZONTAL
#define USE_HORIZONTAL 2
#endif

#if (USE_HORIZONTAL == 0) || (USE_HORIZONTAL == 1)
#define LCD_W 172
#define LCD_H 320
#else
#define LCD_W 320
#define LCD_H 172
#endif

/* 常用颜色（RGB565） */
#define WHITE 0xFFFF
#define BLACK 0x0000
#define RED 0xF800
#define YELLOW  0xffe0
#define GRAY 0x9cf3
#define ORANGE 0xfd00

#define GRAY_UI 0x2965
#define RED_UI 0xF227
#define YELLOW_UI 0xfea9
#define BLUE_UI 0x7d1f
#define BLUE_UI_2 0x055E
#define GREEN_UI 0x5ee7
#define PINK 0xfedb
#define SKYBLUE 0x07fe

#define MAX_SLOTS 16
#define BUF_LEN 32-1
#define INVALID_SLOT_ID 0xFFFFu
/* 空闲槽位标识（若 id 可能为 0，请使用非 0 值） */

/* ==============================================================
   LCD 驱动相关函数
   ============================================================== */
	 
/* 非对外函数不展示 */	 

/* ------------------------------------------------------------------
   初始化硬件函数
   ------------------------------------------------------------------ */
void LCD_Reset(void);
void LCD_Init(uint16_t color);

/* ------------------------------------------------------------------
   控制函数
   ------------------------------------------------------------------ */
void LCD_SpinScreen(uint8_t locate);
void LCD_BLK(uint8_t io);

/* ------------------------------------------------------------------
   基本绘制
   ------------------------------------------------------------------ */
void LCD_Fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color);
void LCD_Clear(uint16_t color);	 

void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);

void LCD_DrawCircle(uint16_t X, uint16_t Y, uint16_t R, uint16_t fc, uint8_t thickness);
void LCD_DrawCircle_Fill(uint16_t X, uint16_t Y, uint16_t R, uint16_t fc);
void LCD_DrawDashedCircle(uint16_t X, uint16_t Y, uint16_t R, uint16_t fc, uint8_t thickness, uint8_t segments);

void LCD_DrawLine(uint16_t x0, uint16_t y0,uint16_t x1, uint16_t y1,uint16_t Color);

void LCD_DrawRect_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bc);
void LCD_DrawTriangel(uint16_t x, uint16_t y, uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color);
void LCD_FillRoundRect(uint16_t x, uint16_t y,
                       uint16_t w, uint16_t h,
                       uint16_t r, uint16_t color);
void LCD_DrawRoundRectStroke(uint16_t x, uint16_t y,
                             uint16_t w, uint16_t h,
                             uint16_t r, uint16_t t,
                             uint16_t color);

/* ------------------------------------------------------------------
  集成绘制
   ------------------------------------------------------------------ */
void LCD_ShowImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *p);
void LCD_ShowPicture(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t pic[]);

typedef struct {
    uint16_t x;        // 左上角 X 坐标
    uint16_t y;        // 左上角 Y 坐标
    uint16_t max_len;  // 最大长度（像素）
    uint16_t height;   // 高度（像素）
    float    min_val;  // 最小值
    float    max_val;  // 最大值
    uint16_t fg_color; // 前景色
    uint16_t bg_color; // 背景色
    uint16_t prev_len; // 上一次绘制的长度
} ProgressBar;

void ProgressBar_Init(ProgressBar *pb, uint16_t x, uint16_t y, uint16_t max_len, uint16_t height, float min_val, float max_val, uint16_t fg_color, uint16_t bg_color);
void ProgressBar_Update(ProgressBar *pb, float cur_val);



/* ------------------------------------------------------------------
  字符绘制
   ------------------------------------------------------------------ */
void LCD_ShowString			  (const char *sample, const unsigned char *data,
												  uint8_t w, uint8_t h,
												  uint16_t x, uint16_t y,
												  uint16_t fc, uint16_t bc,
												  char* str);

void LCD_ShowString_48_64(uint16_t x, uint16_t y,
                          uint16_t fc, uint16_t bc,
                          const char *str);
void LCD_ShowString_32_16(uint16_t x, uint16_t y, 
													uint16_t fc, uint16_t bc,
													const char *s);
void LCD_ShowString_24_12(uint16_t x, uint16_t y, 
                          uint16_t fc, uint16_t bc, 
                          const char *c);
void LCD_ShowString_16_8(uint16_t x,uint16_t y,
													uint16_t fc,uint16_t bc,
													char *c);
void LCD_ShowString_12_6(uint16_t x, uint16_t y,
                         uint16_t fc, uint16_t bc,
                         const char *str);

/* ------------------------------------------------------------------
  加速缓冲
   ------------------------------------------------------------------ */
void slot_module_init(void);
int slot_diff_from_buf(uint16_t id, const char in[BUF_LEN], char out[BUF_LEN]);
int slot_diff_from_cstr(uint16_t id, const char *s, char out[BUF_LEN]);
void slot_clear_id(uint16_t id);
void slot_clear_all(void);


/* ------------------------------------------------------------------
   ST7789 @ SPI
   ------------------------------------------------------------------ */

/* 发送命令 */
__STATIC_INLINE void LCD_SendIndex(uint8_t cmd)
{
    DC_L();   // 命令模式
    CS_L();   // 选中屏幕
//    while ((LL_SPI_IsActiveFlag_TXE(ST7789_SPI) == 0) || (LL_SPI_IsActiveFlag_BSY(ST7789_SPI) != 0)) { }
    LL_SPI_TransmitData8(ST7789_SPI, cmd);
    while ((LL_SPI_IsActiveFlag_TXE(ST7789_SPI) == 0) || (LL_SPI_IsActiveFlag_BSY(ST7789_SPI) != 0)) { }
//		volatile uint32_t tmpreg = SPI1->SR; (void)tmpreg;
    CS_H();
    DC_H();   // 恢复为数据模式
}

/* 发送 8 位数据 */
__STATIC_INLINE void LCD_SendData(uint8_t data)
{
    DC_H();   // 数据模式
    CS_L();
//    while ((LL_SPI_IsActiveFlag_TXE(ST7789_SPI) == 0) || (LL_SPI_IsActiveFlag_BSY(ST7789_SPI) != 0)) { }
    LL_SPI_TransmitData8(ST7789_SPI, data);
    while ((LL_SPI_IsActiveFlag_TXE(ST7789_SPI) == 0) || (LL_SPI_IsActiveFlag_BSY(ST7789_SPI) != 0)) { }
//		volatile uint32_t tmpreg = SPI1->SR; (void)tmpreg;
    CS_H();
}

/* 发送 16 位数据（分两次 8bit） */
__STATIC_INLINE void LCD_Send16Bit(uint16_t data)
{
    DC_H();   // 数据模式
    CS_L();
//    while ((LL_SPI_IsActiveFlag_TXE(ST7789_SPI) == 0) || (LL_SPI_IsActiveFlag_BSY(ST7789_SPI) != 0)) { }
    // 高字节
    LL_SPI_TransmitData8(ST7789_SPI, (uint8_t)(data >> 8));
    while (LL_SPI_IsActiveFlag_TXE(ST7789_SPI) == 0) { }

    // 低字节
    LL_SPI_TransmitData8(ST7789_SPI, (uint8_t)(data & 0xFF));
    while ((LL_SPI_IsActiveFlag_TXE(ST7789_SPI) == 0) || (LL_SPI_IsActiveFlag_BSY(ST7789_SPI) != 0)) { }
//		volatile uint32_t tmpreg = SPI1->SR; (void)tmpreg;
    CS_H();
}



#ifdef __cplusplus
}
#endif

#endif /* __ST7789_H__ */
