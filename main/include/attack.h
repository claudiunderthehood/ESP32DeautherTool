#pragma once

#include <stdint.h>
#include "esp_wifi.h"

extern int deauth_selection;

/* Represents an 802.11 Deauth frame */
typedef struct {
    uint8_t  frame_control[2];
    uint8_t  duration[2];
    uint8_t  station[6];
    uint8_t  sender[6];
    uint8_t  access_point[6];
    uint8_t  fragment_sequence[2];
    uint16_t reason;
} deauth_packet_t;

/* Basic MAC header for promiscuous callback */
typedef struct {
    uint16_t frame_ctrl;
    uint16_t duration;
    uint8_t  dest[6];
    uint8_t  src[6];
    uint8_t  bssid[6];
    uint16_t sequence_ctrl;
    uint8_t  addr4[6];
} mac_header_t;

typedef struct {
    mac_header_t hdr;
    uint8_t   payload[0];
} wifi_packet_t;

/* Promiscuous filter for mgmt+data frames */
static const wifi_promiscuous_filter_t g_filter_config = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA
};

/**
 * @brief Start the deauth process
 *
 * @param wifi_number  Index of the targeted network (if single-target)
 * @param attack_type  DEAUTH_TYPE_SINGLE or DEAUTH_TYPE_ALL
 * @param reason       802.11 reason code to use (0..65535)
 */
void start_attack(int wifi_number, int attack_type, uint16_t reason);

/**
 * @brief Stop the deauth process
 */
void stop_attack(void);
