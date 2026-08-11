/**
 * @file pcap_writer.c
 * @brief Implementasi fungsi penulisan berkas PCAP standar Wireshark untuk ESP32/ESP-IDF
 */

#include "pcap_writer.h"
#include "esp_log.h"

static const char *TAG = "main:pcap_writer";

FILE* pcap_init_file(const char* filepath) {
    if (filepath == NULL) {
        ESP_LOGE(TAG, "Filepath tidak valid (NULL)!");
        return NULL;
    }

    // Buka file dengan mode "wb" (Write Binary) agar karakter baris baru tidak dirubah oleh sistem
    FILE* f = fopen(filepath, "wb"); 
    if (f == NULL) {
        ESP_LOGE(TAG, "Gagal membuat/membuka file PCAP di: %s", filepath);
        return NULL;
    }

    // Inisialisasi Global Header Wireshark (24 Bytes)
    pcap_global_header_t global_header = {
        .magic_number = 0xa1b2c3d4, // Magic number standar PCAP asli
        .version_major = 2,
        .version_minor = 4,
        .thiszone = 0,
        .sigfigs = 0,
        .snaplen = 65535,
        .network = 105 // LINKTYPE_IEEE802_11 (Wajib untuk paket Wi-Fi mentah)
    };

    // Tulis 24-byte Global Header ke awal file
    size_t written = fwrite(&global_header, 1, sizeof(pcap_global_header_t), f);
    if (written != sizeof(pcap_global_header_t)) {
        ESP_LOGE(TAG, "Gagal menulis Global Header secara utuh ke flash!");
        fclose(f);
        return NULL;
    }

    ESP_LOGI(TAG, "File PCAP berhasil diinisialisasi di: %s", filepath);
    return f;
}

void pcap_write_packet(FILE* f, const uint8_t* payload, uint32_t len) {
    if (f == NULL) {
        ESP_LOGE(TAG, "Gagal menulis paket: Pointer file NULL!");
        return;
    }
    if (payload == NULL || len == 0) {
        ESP_LOGW(TAG, "Peringatan: Payload kosong atau panjang data 0!");
        return;
    }

    // Buat bungkus Packet Header PCAP (16 Bytes) untuk frame ini
    pcap_packet_header_t pkt_header = {
        .ts_sec = 0,   // Diisi 0 untuk menghemat daya komputasi CPU ESP32
        .ts_usec = 0,  // Diisi 0 karena Hashcat/Wireshark tidak wajib membaca timestamp presisi
        .incl_len = len,
        .orig_len = len
    };

    // 1. Tulis Packet Header (16 Bytes)
    size_t header_written = fwrite(&pkt_header, 1, sizeof(pcap_packet_header_t), f);
    if (header_written != sizeof(pcap_packet_header_t)) {
        ESP_LOGE(TAG, "Gagal menulis Packet Header ke flash!");
        return;
    }

    // 2. Tulis Data Mentah Paket Wi-Fi (EAPOL payload)
    size_t payload_written = fwrite(payload, 1, len, f);
    if (payload_written != len) {
        ESP_LOGE(TAG, "Gagal menulis payload paket secara utuh ke flash! (Tertulis %d dari %d bytes)", payload_written, len);
        return;
    }
    
    ESP_LOGD(TAG, "Satu paket Wi-Fi (%d bytes) berhasil ditulis ke PCAP.", len);
}

void pcap_close_file(FILE* f) {
    if (f == NULL) {
        ESP_LOGW(TAG, "Fungsi close dipanggil pada pointer file NULL.");
        return;
    }

    // Paksa seluruh sisa data bita yang mengendap di buffer I/O RAM untuk masuk ke dalam IC Flash Memory
    fflush(f); 
    
    // Kunci berkas secara permanen di sistem penyimpanan
    int res = fclose(f);
    if (res == 0) {
        ESP_LOGI(TAG, "Berkas PCAP berhasil dikunci dan disimpan 100%% aman ke Flash Storage.");
    } else {
        ESP_LOGE(TAG, "Gagal mengunci berkas PCAP pada proses pembongkaran memori!");
    }
}
