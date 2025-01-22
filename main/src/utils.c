#include "config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void blink_led(int num_times, int blink_duration_ms) {
#if (LED_GPIO >= 0)
    for(int i = 0; i < num_times; i++) {
        gpio_set_level(LED_GPIO, LED_ON);
        vTaskDelay(pdMS_TO_TICKS(blink_duration_ms / 2));
        gpio_set_level(LED_GPIO, LED_OFF);
        vTaskDelay(pdMS_TO_TICKS(blink_duration_ms / 2));
    }
#endif
}