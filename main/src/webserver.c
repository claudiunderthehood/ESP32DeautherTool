#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_http_server.h"

#include "webserver.h"
#include "config.h"
#include "attack.h"

#define MAX_AP_RECORDS 20
#define BUF_SIZE 4096

wifi_ap_record_t g_ap_list[MAX_AP_RECORDS];
int g_ap_count = 0;

static httpd_handle_t g_http_server = NULL;

static esp_err_t handle_root(httpd_req_t *req);
static esp_err_t handle_attack(httpd_req_t *req);
static esp_err_t handle_attack_all(httpd_req_t *req);
static esp_err_t handle_rescan(httpd_req_t *req);
static esp_err_t handle_stop(httpd_req_t *req);

/**
 * @brief Convert wifi_auth_mode_t to string
 * 
 * @param auth Wi-Fi auth mode.
 * @return const char* Auth mode as string.
 */
static const char *get_auth_mode_name(wifi_auth_mode_t auth) {
    switch(auth) {
        case WIFI_AUTH_OPEN:            return "Open";
        case WIFI_AUTH_WEP:             return "WEP";
        case WIFI_AUTH_WPA_PSK:         return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK:        return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
        default:                        return "UNKNOWN";
    }
}

/**
 * @brief Minimal POST parser that returns int from param_name=.
 * 
 * @param req HTTP request.
 * @param param_name Name of the parameter to extract.
 * @return int Extracted integer or -1 on error.
 */
static int get_post_param_int(httpd_req_t *req, const char *param_name) {
    char buf[300];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);

    if(len <= 0) return -1;

    buf[len] = '\0';

    char *p = strstr(buf, param_name);

    if(!p) return -1;

    p += strlen(param_name) + 1; // skip "param_name="
    return atoi(p);
}

/**
 * @brief Do a Wi-Fi scan in AP+STA mode so scanning works reliably in IDF.
 */
static void do_scan(void) {
    wifi_mode_t current_mode;
    esp_wifi_get_mode(&current_mode);

    if(current_mode == WIFI_MODE_AP) esp_wifi_set_mode(WIFI_MODE_APSTA);

    wifi_scan_config_t scan_conf = {
        .ssid       = NULL,
        .bssid      = NULL,
        .channel    = 0,
        .scan_type  = WIFI_SCAN_TYPE_ACTIVE,
        .show_hidden= true
    };

    esp_wifi_scan_stop();
    if(esp_wifi_scan_start(&scan_conf, true) == ESP_OK) {
        uint16_t n = MAX_AP_RECORDS;
        if(esp_wifi_scan_get_ap_records(&n, g_ap_list) == ESP_OK) {
            g_ap_count = n;
            ESP_LOGI(TAG, "Scan done, found %d APs", g_ap_count);
        }
    }
}

/**
 * @brief Handles the root page.
 * 
 * @param req The HTTP request.
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error.
 */
static esp_err_t handle_root(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");

    char *buf = malloc(BUF_SIZE);
    if(!buf) return httpd_resp_send_500(req);

    const char *html_header =
        "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<title>ESP32 Deauthentication Tool</title>"
        "<style>"
        "body { background: #222; color: #eee; font-family: Arial; max-width: 800px; margin: auto; padding: 20px; }"
        "table { width: 100%; border-collapse: collapse; margin-bottom: 20px; }"
        "th, td { padding: 8px; border-bottom: 1px solid #444; }"
        "th { background: #444; }"
        ".btn { background: #3498db; color: #fff; padding: 10px; border: none; margin: 8px 0; cursor: pointer; }"
        ".btn:hover { background: #2980b9; }"
        "input { padding: 6px; margin: 4px 0; }"
        "</style>"
        "</head><body>";

    httpd_resp_sendstr_chunk(req, html_header);
    httpd_resp_sendstr_chunk(req, "<h1>ESP32 Deauthentication Tool</h1><h2>Nearby WiFi Networks</h2>");

    snprintf(buf, BUF_SIZE, "<table><tr><th>#</th><th>SSID</th><th>BSSID</th><th>Ch</th><th>RSSI</th><th>Enc</th></tr>");
    httpd_resp_sendstr_chunk(req, buf);

    for(int i = 0; i < g_ap_count; i++) {
        wifi_ap_record_t *ap = &g_ap_list[i];
        snprintf(buf, BUF_SIZE,
            "<tr>"
              "<td>%d</td>"
              "<td>%s</td>"
              "<td>%02X:%02X:%02X:%02X:%02X:%02X</td>"
              "<td>%d</td>"
              "<td>%d</td>"
              "<td>%s</td>"
            "</tr>",
            i,
            (char *)ap->ssid,
            ap->bssid[0], ap->bssid[1], ap->bssid[2],
            ap->bssid[3], ap->bssid[4], ap->bssid[5],
            ap->primary,
            ap->rssi,
            get_auth_mode_name(ap->authmode)
        );
        httpd_resp_sendstr_chunk(req, buf);
    }
    httpd_resp_sendstr_chunk(req, "</table>");

    httpd_resp_sendstr_chunk(req,
        "<form method='POST' action='/rescan'>"
        "<input class='btn' type='submit' value='New Scan'>"
        "</form>"
    );

    httpd_resp_sendstr_chunk(req, 
        "<form method='POST' action='/attack'>"
        "<h3>Start Deauth Attack (Single)</h3>"
        "<p>Select AP:</p>"
        "<table>"
    );

    for(int i = 0; i < g_ap_count; i++) {
        wifi_ap_record_t *ap = &g_ap_list[i];
        snprintf(buf, BUF_SIZE,
            "<tr>"
              "<td><input type='radio' name='ap_select' value='%d'></td>"
              "<td>%s (%02X:%02X:%02X:%02X:%02X:%02X) [Ch %d]</td>"
            "</tr>",
            i,
            (char*)ap->ssid,
            ap->bssid[0], ap->bssid[1], ap->bssid[2],
            ap->bssid[3], ap->bssid[4], ap->bssid[5],
            ap->primary
        );
        httpd_resp_sendstr_chunk(req, buf);
    }

    httpd_resp_sendstr_chunk(req,
        "</table>"
        "<p>Reason code: <input type='text' name='reason'></p>"
        "<button class='btn' type='submit'>Start Attack</button>"
        "</form>"
    );

    httpd_resp_sendstr_chunk(req,
        "<form method='POST' action='/attack_all'>"
        "<h3>Jam all the Wi-Fi Networks</h3>"
        "<p>Reason code: <input type='text' name='reason'></p>"
        "<button class='btn' type='submit'>Attack All</button>"
        "</form>"
    );

    httpd_resp_sendstr_chunk(req,
        "<form method='POST' action='/stop'>"
        "<button class='btn' type='submit'>Stop Attack</button>"
        "</form>"
    );

    httpd_resp_sendstr_chunk(req,
        "<hr><h4>Reason Codes</h4>"
        "<table><tr><th>Code</th><th>Meaning</th></tr>"
        "<tr><td>0</td><td>Reserved</td></tr>"
        "<tr><td>1</td><td>Unspecified reason</td></tr>"
        "<tr><td>3</td><td>STA leaving BSS</td></tr>"
        "<tr><td>4</td><td>Inactivity</td></tr>"
        "<tr><td>5</td><td>Too many STAs</td></tr>"
        "</table>"
        "</body></html>"
    );

    httpd_resp_sendstr_chunk(req, NULL);
    free(buf);
    return ESP_OK;
}

/**
 * @brief POST /attack => single-target. 
 *        We parse "ap_select" (radio button) & "reason" like we did for net_num.
 * 
 * @param req The HTTP request.
 * @return esp_err_t ESP_OK on success.
 */
static esp_err_t handle_attack(httpd_req_t *req) {
    int net_num  = get_post_param_int(req, "ap_select");
    int reason_i = get_post_param_int(req, "reason");
    uint16_t reason = (uint16_t)(reason_i & 0xFFFF);

    if((net_num >= 0) && (net_num < g_ap_count)) {
        start_attack(net_num, DEAUTH_SINGLE, reason);

        char resp[256];
        snprintf(resp, sizeof(resp),
            "<html><body><h2>Starting Attack (Single)</h2>"
            "<p>AP Index: %d, Reason: %u</p>"
            "<a href='/'>Back</a></body></html>",
            net_num, reason
        );
        httpd_resp_sendstr(req, resp);
    }else{
        httpd_resp_sendstr(req,
            "<html><body><h2>Error: Invalid AP Selection</h2>"
            "<a href='/'>Back</a></body></html>"
        );
    }
    return ESP_OK;
}

/**
 * @brief POST /attack_all => all-network attack.
 * 
 * @param req THE HTTP request.
 * @return esp_err_t ESP_OK on success.
 */
static esp_err_t handle_attack_all(httpd_req_t *req) {
    int reason_i = get_post_param_int(req, "reason");
    uint16_t reason = (uint16_t)(reason_i & 0xFFFF);

    start_attack(0, DEAUTH_ALL, reason);

    char resp[256];
    snprintf(resp, sizeof(resp),
        "<html><body>"
        "<h2>Jam all the Wi-Fi networks.</h2>"
        "<p>Reason: %u</p>"
        "<p>To stop, press Stop Attack or reset the device.</p>"
        "<a href='/'>Back</a>"
        "</body></html>",
        reason
    );
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

/**
 * @brief POST /rescan => do a new Wi-Fi scan, then redirect to /.
 * 
 * @param req The HTTP request.
 * @return esp_err_t ESP_OK on success.
 */
static esp_err_t handle_rescan(httpd_req_t *req) {
    do_scan();
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "");
    return ESP_OK;
}

/**
 * @brief POST /stop => stop the attack.
 * 
 * @param req The HTTP request.
 * @return esp_err_t ESP_OK on success.
 */
static esp_err_t handle_stop(httpd_req_t *req) {
    stop_attack();
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "");
    return ESP_OK;
}

void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    if(httpd_start(&g_http_server, &config) == ESP_OK) {
        httpd_uri_t root_uri = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = handle_root,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(g_http_server, &root_uri);

        httpd_uri_t attack_uri = {
            .uri      = "/attack",
            .method   = HTTP_POST,
            .handler  = handle_attack,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(g_http_server, &attack_uri);

        httpd_uri_t attack_all_uri = {
            .uri      = "/attack_all",
            .method   = HTTP_POST,
            .handler  = handle_attack_all,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(g_http_server, &attack_all_uri);

        httpd_uri_t rescan_uri = {
            .uri      = "/rescan",
            .method   = HTTP_POST,
            .handler  = handle_rescan,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(g_http_server, &rescan_uri);

        httpd_uri_t stop_uri = {
            .uri      = "/stop",
            .method   = HTTP_POST,
            .handler  = handle_stop,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(g_http_server, &stop_uri);

        ESP_LOGI(TAG, "Web server started on port 80");

        do_scan();
    }else{
        ESP_LOGE(TAG, "Could not start web server");
    }
}

void stop_webserver(void) {
    if(g_http_server) {
        httpd_stop(g_http_server);
        g_http_server = NULL;
    }
}