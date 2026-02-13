/*

//闪灯
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
        printf("GPIO8 ON\n");
        gpio_set_level(GPIO_NUM_8, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("GPIO8 OFF\n");
        gpio_set_level(GPIO_NUM_8, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

*/


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_8

void task1(void *pvParameters) {
    while (1) {
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void task2(void *pvParameters) {
    while (1) {
        printf("Hello from Task2!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    // 配置 LED GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // 创建两个任务
    xTaskCreate(task1, "Task1", 2048, NULL, 5, NULL);
    xTaskCreate(task2, "Task2", 2048, NULL, 5, NULL);
}
