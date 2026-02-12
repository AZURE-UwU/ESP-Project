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
