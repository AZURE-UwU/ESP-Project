/* 串口打印 */

/* 
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    printf("Hello world!\n");

    while (1) {
        printf("Hello again!\n");
        vTaskDelay(pdMS_TO_TICKS(1000)); // 每隔 1 秒打印一次
    }
}
 */



/* 闪灯 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_8),   // 指定 GPIO8
        .mode = GPIO_MODE_OUTPUT,               // 设置为输出模式
        .pull_up_en = GPIO_PULLUP_DISABLE,      // 不使能上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  // 不使能下拉
        .intr_type = GPIO_INTR_DISABLE          // 不使用中断
    };
    gpio_config(&io_conf);

    while (1) {
        gpio_set_level(GPIO_NUM_8, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(GPIO_NUM_8, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
