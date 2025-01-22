#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "driver/gpio.h"

#include "config.h"
#include "webserver.h"
#include "attack.h"

/**
 * @brief Main loop task for channel hopping in the DEAUTH_ALL mode.
 * 
 * @param arg Task argument.
 */
static void main_loop_task(void *arg) {
    int curr_channel = 1;
    while(true) {
        if(deauth_selection == DEAUTH_ALL) {

            if(curr_channel > CAP_CHANNEL) curr_channel = 1;

            esp_wifi_set_channel(curr_channel, WIFI_SECOND_CHAN_NONE);
            curr_channel++;
            vTaskDelay(pdMS_TO_TICKS(10));
        }else{
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_ap();

#if(LED_GPIO >= 0)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = 0,
        .pull_down_en = 0,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, LED_OFF);
#endif

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t ap_config = {0};
    strncpy((char*)ap_config.ap.ssid, AP_SSID, sizeof(ap_config.ap.ssid));
    strncpy((char*)ap_config.ap.password, AP_PASS, sizeof(ap_config.ap.password));
    ap_config.ap.ssid_len       = strlen(AP_SSID);
    ap_config.ap.channel        = 1;
    ap_config.ap.authmode       = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.max_connection = 4;
    ap_config.ap.beacon_interval= 100;
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi AP started. SSID=%s PASS=%s", AP_SSID, AP_PASS);

    start_webserver();

    xTaskCreate(main_loop_task, "main_loop_task", 4096, NULL, 1, NULL);
}