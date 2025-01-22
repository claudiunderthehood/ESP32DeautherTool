#pragma once

#include "esp_log.h"
#include "driver/gpio.h"

#define AP_SSID  "ESP32Deauther"
#define AP_PASS  "password123"

#define LED_GPIO 2
#define LED_ON   1
#define LED_OFF  0

#define CAP_CHANNEL 13
#define FRAMES_NUMBER 16

#define LINK_REPS 2
#define BLINK_DELAY 20

#define DEAUTH_SINGLE 0
#define DEAUTH_ALL 1

#define TAG "DEAUTHER"

/**
 * @brief Utility function to blink led.
 * 
 * @param num_times Number of times to blink the led.
 * @param blink_duration_ms Duration of the blinking (ms)
 */
void blink_led(int num_times, int blink_duration_ms);