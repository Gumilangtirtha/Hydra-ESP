#include "wifi_controller.h"

#include <stdio.h>
#include <string.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "nvs_flash.h"
#include "nvs.h"

// Tambahan pustaka generator acak hardware ESP32
#include "esp_random.h"
// Pustaka ADC untuk True Random Entropy
#include "driver/adc.h"

static const char* TAG = "wifi_controller";

// Variabel global untuk menyimpan channel AP yang aktif
uint8_t current_ap_channel = 0;

/**
 * @brief Stores current state of Wi-Fi interface
 */
static bool wifi_init = false;
static uint8_t original_mac_ap[6];

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data){

}

/**
 * @brief Mengubah MAC Address AP agar terdeteksi mutlak sebagai vendor ZTE ORIGINAL.
 *        Memilih satu dari 200 array OUI resmi ZTE Corporation secara acak, 
 *        sedangkan 3 byte sisanya diacak total menggunakan hardware ESP32.
 */
void wifictl_set_vendor_zte_random_mac() {
    uint8_t current_mac[6];
    
    // Array Statis 200 OUI MAC Prefix Resmi ZTE Corporation.
    // Diformat dari 20 OUI ZTE paling valid yang diduplikasi secara aman.
    static const uint8_t zte_ouis[200][3] = {
        // Blok 1
        {0xE8, 0xE7, 0xC3}, {0x34, 0x4B, 0x50}, {0xCC, 0x1A, 0xFA}, {0xDC, 0x02, 0x8E}, {0xD4, 0x37, 0xF7},
        {0x20, 0x89, 0x84}, {0x4C, 0x16, 0xF1}, {0x78, 0x31, 0x2B}, {0x98, 0xF8, 0xDB}, {0x00, 0x1E, 0x73},
        {0x00, 0x22, 0x93}, {0x2C, 0x26, 0xC5}, {0x34, 0xE0, 0xCF}, {0x38, 0x0A, 0x94}, {0x4C, 0xAC, 0x0A},
        {0x54, 0x22, 0xF8}, {0x68, 0x1A, 0xB2}, {0x84, 0x74, 0x2A}, {0x8C, 0xE5, 0x83}, {0xA0, 0xEC, 0x80},
        // Blok 2
        {0xE8, 0xE7, 0xC3}, {0x34, 0x4B, 0x50}, {0xCC, 0x1A, 0xFA}, {0xDC, 0x02, 0x8E}, {0xD4, 0x37, 0xF7},
        {0x20, 0x89, 0x84}, {0x4C, 0x16, 0xF1}, {0x78, 0x31, 0x2B}, {0x98, 0xF8, 0xDB}, {0x00, 0x1E, 0x73},
        {0x00, 0x22, 0x93}, {0x2C, 0x26, 0xC5}, {0x34, 0xE0, 0xCF}, {0x38, 0x0A, 0x94}, {0x4C, 0xAC, 0x0A},
        {0x54, 0x22, 0xF8}, {0x68, 0x1A, 0xB2}, {0x84, 0x74, 0x2A}, {0x8C, 0xE5, 0x83}, {0xA0, 0xEC, 0x80},
        // Blok 3
        {0xE8, 0xE7, 0xC3}, {0x34, 0x4B, 0x50}, {0xCC, 0x1A, 0xFA}, {0xDC, 0x02, 0x8E}, {0xD4, 0x37, 0xF7},
        {0x20, 0x89, 0x84}, {0x4C, 0x16, 0xF1}, {0x78, 0x31, 0x2B}, {0x98, 0xF8, 0xDB}, {0x00, 0x1E, 0x73},
        {0x00, 0x22, 0x93}, {0x2C, 0x26, 0xC5}, {0x34, 0xE0, 0xCF}, {0x38, 0x0A, 0x94}, {0x4C, 0xAC, 0x0A},
        {0x54, 0x22, 0xF8}, {0x68, 0x1A, 0xB2}, {0x84, 0x74, 0x2A}, {0x8C, 0xE5, 0x83}, {0xA0, 0xEC, 0x80},
        // Blok 4
        {0xE8, 0xE7, 0xC3}, {0x34, 0x4B, 0x50}, {0xCC, 0x1A, 0xFA}, {0xDC, 0x02, 0x8E}, {0xD4, 0x37, 0xF7},
        {0x20, 0x89, 0x84}, {0x4C, 0x16, 0xF1}, {0x78, 0x31, 0x2B}, {0x98, 0xF8, 0xDB}, {0x00, 0x1E, 0x73},
        {0x00, 0x22, 0x93}, {0x2C, 0x26, 0xC5}, {0x34, 0xE0, 0xCF}, {0x38, 0x0A, 0x94}, {0x4C, 0xAC, 0x0A},
        {0x54, 0x22, 0xF8}, {0x68, 0x1A, 0xB2}, {0x84, 0x74, 0x2A}, {0x8C, 0xE5, 0x83}, {0xA0, 0xEC, 0x80},
        // Blok 5
        {0xE8, 0xE7, 0xC3}, {0x34, 0x4B, 0x50}, {0xCC, 0x1A, 0xFA}, {0xDC, 0x02, 0x8E}, {0xD4, 0x37, 0xF7},
        {0x20, 0x89, 0x84}, {0x4C, 0x16, 0xF1}, {0x78, 0x31, 0x2B}, {0x98, 0xF8, 0xDB}, {0x00, 0x1E, 0x73},
        {0x00, 0x22, 0x93}, {0x2C, 0x26, 0xC5}, {0x34, 0xE0, 0xCF}, {0x38, 0x0A, 0x94}, {0x4C, 0xAC, 0x0A},
        {0x54, 0x22, 0xF8}, {0x68, 0x1A, 0xB2}, {0x84, 0x74, 0x2A}, {0x8C, 0xE5, 0x83}, {0xA0, 0xEC, 0x80},
        // Blok 6
        {0xE8, 0xE7, 0xC3}, {0x34, 0x4B, 0x50}, {0xCC, 0x1A, 0xFA}, {0xDC, 0x02, 0x8E}, {0xD4, 0x37, 0xF7},
        {0x20, 0x89, 0x84}, {0x4C, 0x16, 0xF1}, {0x78, 0x31, 0x2B}, {0x98, 0xF8, 0xDB}, {0x00, 0x1E, 0x73},
        {0x00, 0x22, 0x93}, {0x2C, 0x26, 0xC5}, {0x34, 0xE0, 0xCF}, {0x38, 0x0A, 0x94}, {0x4C, 0xAC, 0x0A},
        {0x54, 0x22, 0xF8}, {0x68, 0x1A, 0xB2}, {0x84, 0x74, 0x2A}, {0x8C, 0xE5, 0x83}, {0xA0, 0xEC, 0x80},
        // Blok 7
        {0xE8, 0xE7, 0xC3}, {0x34, 0x4B, 0x50}, {0xCC, 0x1A, 0xFA}, {0xDC, 0x02, 0x8E}, {0xD4, 0x37, 0xF7},
        {0x20, 0x89, 0x84}, {0x4C, 0x16, 0xF1}, {0x78, 0x31, 0x2B}, {0x98, 0xF8, 0xDB}, {0x00, 0x1E, 0x73},
        {0x00, 0x22, 0x93}, {0x2C, 0x26, 0xC5}, {0x34, 0xE0, 0xCF}, {0x38, 0x0A, 0x94}, {0x4C, 0xAC, 0x0A},
        {0x54, 0x22, 0xF8}, {0x68, 0x1A, 0xB2}, {0x84, 0x74, 0x2A}, {0x8C, 0xE5, 0x83}, {0xA0, 0xEC, 0x80},
        // Blok 8
        {0xE8, 0xE7, 0xC3}, {0x34, 0x4B, 0x50}, {0xCC, 0x1A, 0xFA}, {0xDC, 0x02, 0x8E}, {0xD4, 0x37, 0xF7},
        {0x20, 0x89, 0x84}, {0x4C, 0x16, 0xF1}, {0x78, 0x31, 0x2B}, {0x98, 0xF8, 0xDB}, {0x00, 0x1E, 0x73},
        {0x00, 0x22, 0x93}, {0x2C, 0x26, 0xC5}, {0x34, 0xE0, 0xCF}, {0x38, 0x0A, 0x94}, {0x4C, 0xAC, 0x0A},
        {0x54, 0x22, 0xF8}, {0x68, 0x1A, 0xB2}, {0x84, 0x74, 0x2A}, {0x8C, 0xE5, 0x83}, {0xA0, 0xEC, 0x80},
        // Blok 9
        {0xE8, 0xE7, 0xC3}, {0x34, 0x4B, 0x50}, {0xCC, 0x1A, 0xFA}, {0xDC, 0x02, 0x8E}, {0xD4, 0x37, 0xF7},
        {0x20, 0x89, 0x84}, {0x4C, 0x16, 0xF1}, {0x78, 0x31, 0x2B}, {0x98, 0xF8, 0xDB}, {0x00, 0x1E, 0x73},
        {0x00, 0x22, 0x93}, {0x2C, 0x26, 0xC5}, {0x34, 0xE0, 0xCF}, {0x38, 0x0A, 0x94}, {0x4C, 0xAC, 0x0A},
        {0x54, 0x22, 0xF8}, {0x68, 0x1A, 0xB2}, {0x84, 0x74, 0x2A}, {0x8C, 0xE5, 0x83}, {0xA0, 0xEC, 0x80},
        // Blok 10
        {0xE8, 0xE7, 0xC3}, {0x34, 0x4B, 0x50}, {0xCC, 0x1A, 0xFA}, {0xDC, 0x02, 0x8E}, {0xD4, 0x37, 0xF7},
        {0x20, 0x89, 0x84}, {0x4C, 0x16, 0xF1}, {0x78, 0x31, 0x2B}, {0x98, 0xF8, 0xDB}, {0x00, 0x1E, 0x73},
        {0x00, 0x22, 0x93}, {0x2C, 0x26, 0xC5}, {0x34, 0xE0, 0xCF}, {0x38, 0x0A, 0x94}, {0x4C, 0xAC, 0x0A},
        {0x54, 0x22, 0xF8}, {0x68, 0x1A, 0xB2}, {0x84, 0x74, 0x2A}, {0x8C, 0xE5, 0x83}, {0xA0, 0xEC, 0x80}
    };

    // 1. Ambil MAC Address interface AP yang saat ini sedang aktif
    if (esp_wifi_get_mac(WIFI_IF_AP, current_mac) == ESP_OK) {
        
        // 2. Pilih salah satu dari 200 Prefix OUI ZTE secara acak
        uint32_t random_prefix_index = esp_random() % 200;
        current_mac[0] = zte_ouis[random_prefix_index][0];
        current_mac[1] = zte_ouis[random_prefix_index][1];
        current_mac[2] = zte_ouis[random_prefix_index][2];
        
        // 3. Acak 3 Byte sisanya secara penuh menggunakan hardware generator ESP32
        current_mac[3] = esp_random() % 256;
        current_mac[4] = esp_random() % 256;
        current_mac[5] = esp_random() % 256;
        
        // 4. Terapkan MAC Address baru ke sistem wifi
        esp_err_t err = esp_wifi_set_mac(WIFI_IF_AP, current_mac);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Vendor MAC sukses diubah ke ZTE Acak -> %02X:%02X:%02X:%02X:%02X:%02X",
                     current_mac[0], current_mac[1], current_mac[2], 
                     current_mac[3], current_mac[4], current_mac[5]);
        } else {
            ESP_LOGE(TAG, "Gagal mengubah MAC Address ke vendor ZTE (Error: %s)", esp_err_to_name(err));
        }
    }
}

/**
 * @brief Initializes Wi-Fi interface into APSTA mode and starts it.
 *
 * @attention This function should be called only once.
 */
static void wifi_init_apsta(){
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, original_mac_ap));

    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_init = true;
}

void wifictl_ap_start(wifi_config_t *wifi_config) {
    ESP_LOGD(TAG, "Starting AP...");
    if(!wifi_init){
        wifi_init_apsta();
    }

    // Jika channel belum diatur (bernilai 0 saat booting), gunakan True Random (ADC Noise)
    if (current_ap_channel == 0) {
        // 1. Ambil kebisingan voltase (noise) dari pin ADC1_CHANNEL_0
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
        int noise_seed = adc1_get_raw(ADC1_CHANNEL_0);

        // 2. Kombinasikan noise ADC dengan esp_random() untuk entropi murni
        uint32_t true_random_value = esp_random() + noise_seed;

        // 3. Batasi hasil angka acak agar selalu berada di rentang Channel 1 sampai Channel 11
        current_ap_channel = (true_random_value % 11) + 1; 

        // Log ke serial monitor
        ESP_LOGI(TAG, "[WIFI CONTROL] True Random Channel Berhasil Dihasilkan: CH %d (Noise Seed: %d)", current_ap_channel, noise_seed);
    }
    
    // Terapkan channel acak ke konfigurasi Wi-Fi
    wifi_config->ap.channel = current_ap_channel;

    // Memasukkan konfigurasi wifi ke hardware AP
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, wifi_config));
    
    // --> MENGUNCI BANDWIDTH KE 20MHz AGAR TIDAK MUNCUL CH GANDA SEPERTI CH 2(4) <--
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20));
    
    // Menjalankan fungsi pengacak MAC Address ZTE Original
    wifictl_set_vendor_zte_random_mac();

    ESP_LOGI(TAG, "AP started with SSID=%s, Channel=%d", wifi_config->ap.ssid, wifi_config->ap.channel);
}

void wifictl_ap_stop(){
    ESP_LOGD(TAG, "Stopping AP...");
    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = 0
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGD(TAG, "AP stopped");
}


void wifictl_sta_connect_to_ap(const wifi_ap_record_t *ap_record, const char password[]){
    ESP_LOGD(TAG, "Connecting STA to AP...");
    if(!wifi_init){
        wifi_init_apsta();
    }

    wifi_config_t sta_wifi_config = {
        .sta = {
            .channel = ap_record->primary,
            .scan_method = WIFI_FAST_SCAN,
            .pmf_cfg.capable = false,
            .pmf_cfg.required = false
        },
    };
    memcpy(sta_wifi_config.sta.ssid, ap_record->ssid, 32);

    if(password != NULL){
        if(strlen(password) >= 64) {
            ESP_LOGE(TAG, "Password is too long. Max supported length is 64");
            return;
        }
        memcpy(sta_wifi_config.sta.password, password, strlen(password) + 1);
    }

    ESP_LOGD(TAG, ".ssid=%s", sta_wifi_config.sta.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

}

void wifictl_sta_disconnect(){
    ESP_ERROR_CHECK(esp_wifi_disconnect());
}

void wifictl_set_ap_mac(const uint8_t *mac_ap){
    ESP_LOGD(TAG, "Changing AP MAC address...");
    ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, mac_ap));
}

void wifictl_get_ap_mac(uint8_t *mac_ap){
    esp_wifi_get_mac(WIFI_IF_AP, mac_ap);
}

void wifictl_restore_ap_mac(){
    ESP_LOGD(TAG, "Restoring original AP MAC address...");
    ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, original_mac_ap));
}

void wifictl_get_sta_mac(uint8_t *mac_sta){
    esp_wifi_get_mac(WIFI_IF_STA, mac_sta);
}

void wifictl_set_channel(uint8_t channel){
    if((channel == 0) || (channel >  13)){
        ESP_LOGE(TAG,"Channel out of range. Expected value from <1,13> but got %u", channel);
        return;
    }
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

esp_err_t wifictl_get_mgmt_creds(char *out_ssid, size_t ssid_len, char *out_password, size_t password_len) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs);
    if (err == ESP_OK) {
        // Try to read ssid
        size_t required = ssid_len;
        esp_err_t r = nvs_get_str(nvs, "mgmt_ssid", out_ssid, &required);
        if (r == ESP_OK) {
            if (required == 0) {
                // empty value -> fallback
                strncpy(out_ssid, CONFIG_MGMT_AP_SSID, ssid_len);
                out_ssid[ssid_len - 1] = '\0';
            }
        } else if (r == ESP_ERR_NVS_NOT_FOUND) {
            strncpy(out_ssid, CONFIG_MGMT_AP_SSID, ssid_len);
            out_ssid[ssid_len - 1] = '\0';
        } else if (r == ESP_ERR_NVS_INVALID_LENGTH) {
            // buffer too small: ensure truncation
            // attempt to read required length then truncate
            size_t actual = 0;
            nvs_get_str(nvs, "mgmt_ssid", NULL, &actual);
            if (actual > 0) {
                size_t to_copy = ssid_len - 1;
                nvs_get_str(nvs, "mgmt_ssid", out_ssid, &required);
                out_ssid[to_copy] = '\0';
            } else {
                strncpy(out_ssid, CONFIG_MGMT_AP_SSID, ssid_len);
                out_ssid[ssid_len - 1] = '\0';
            }
        } else {
            strncpy(out_ssid, CONFIG_MGMT_AP_SSID, ssid_len);
            out_ssid[ssid_len - 1] = '\0';
        }

        // Try to read password
        required = password_len;
        r = nvs_get_str(nvs, "mgmt_password", out_password, &required);
        if (r == ESP_OK) {
            if (required == 0) {
                strncpy(out_password, CONFIG_MGMT_AP_PASSWORD, password_len);
                out_password[password_len - 1] = '\0';
            }
        } else if (r == ESP_ERR_NVS_NOT_FOUND) {
            strncpy(out_password, CONFIG_MGMT_AP_PASSWORD, password_len);
            out_password[password_len - 1] = '\0';
        } else if (r == ESP_ERR_NVS_INVALID_LENGTH) {
            size_t actual = 0;
            nvs_get_str(nvs, "mgmt_password", NULL, &actual);
            if (actual > 0) {
                size_t to_copy = password_len - 1;
                nvs_get_str(nvs, "mgmt_password", out_password, &required);
                out_password[to_copy] = '\0';
            } else {
                strncpy(out_password, CONFIG_MGMT_AP_PASSWORD, password_len);
                out_password[password_len - 1] = '\0';
            }
        } else {
            strncpy(out_password, CONFIG_MGMT_AP_PASSWORD, password_len);
            out_password[password_len - 1] = '\0';
        }

        nvs_close(nvs);
        return ESP_OK;
    } else {
        // NVS not available -> fallback to build-time defaults
        strncpy(out_ssid, CONFIG_MGMT_AP_SSID, ssid_len);
        out_ssid[ssid_len - 1] = '\0';
        strncpy(out_password, CONFIG_MGMT_AP_PASSWORD, password_len);
        out_password[password_len - 1] = '\0';
        return err;
    }
}


void wifictl_mgmt_ap_start() {
    esp_wifi_set_mode(WIFI_MODE_APSTA);

    char current_ssid[32] = {0};
    char current_pass[64] = {0};
    wifictl_get_mgmt_creds(current_ssid, sizeof(current_ssid), current_pass, sizeof(current_pass));

    wifi_config_t mgmt_wifi_config = {
        .ap = {
            .ssid_len = strlen(current_ssid),
            .channel = current_ap_channel > 0 ? current_ap_channel : 1, // Pastikan konsisten menggunakan True Random
            .max_connection = CONFIG_MGMT_AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    memcpy(mgmt_wifi_config.ap.ssid, current_ssid, 32);
    memcpy(mgmt_wifi_config.ap.password, current_pass, 64);

    wifictl_ap_start(&mgmt_wifi_config);
}

void wifictl_mgmt_ap_stop(){
    ESP_LOGW(TAG, "Stopping Management AP...");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}
