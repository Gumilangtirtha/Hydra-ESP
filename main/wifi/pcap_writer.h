/**
 * @file pcap_writer.h
 * @brief Provides standard Wireshark PCAP Global and Packet Header structures for ESP32/ESP-IDF
 */

#ifndef PCAP_WRITER_H
#define PCAP_WRITER_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Struktur Global Header PCAP standar Wireshark (24 Bytes)
 * Menggunakan atribut packed agar compiler tidak menambahkan padding memori otomatis
 */
typedef struct __attribute__((packed)) {
    uint32_t magic_number;   // 0xa1b2c3d4 (Penanda format PCAP asli)
    uint16_t version_major;  // 2
    uint16_t version_minor;  // 4
    int32_t  thiszone;       // GMT to local correction (set 0)
    uint32_t sigfigs;        // Accuracy of timestamps (set 0)
    uint32_t snaplen;        // Max packet capture size (65535)
    uint32_t network;        // LINKTYPE_IEEE802_11 = 105 (Kode wajib untuk Wi-Fi mentah)
} pcap_global_header_t;

/**
 * @brief Struktur Packet Header PCAP untuk setiap frame yang masuk (16 Bytes)
 */
typedef struct __attribute__((packed)) {
    uint32_t ts_sec;         // Timestamp dalam satuan Detik
    uint32_t ts_usec;        // Timestamp dalam satuan Mikrodetik
    uint32_t incl_len;       // Ukuran bita paket yang disimpan di storage
    uint32_t orig_len;       // Ukuran asli bita paket saat berada di udara
} pcap_packet_header_t;

/**
 * @brief Menginisialisasi berkas PCAP baru di sistem penyimpanan SPIFFS/LittleFS
 * Fungsi ini otomatis menulis 24-byte Global Header di awal berkas.
 * 
 * @param filepath Lokasi penyimpanan file (contoh: "/data/handshake.pcap")
 * @return FILE* Pointer ke objek berkas yang berhasil dibuat, atau NULL jika gagal.
 */
FILE* pcap_init_file(const char* filepath);

/**
 * @brief Membungkus paket mentah dengan PCAP Packet Header dan menulisnya ke storage
 * 
 * @param f Pointer berkas PCAP yang aktif
 * @param payload Pointer data biner mentah (EAPOL frame)
 * @param len Ukuran panjang data biner yang akan ditulis
 */
void pcap_write_packet(FILE* f, const uint8_t* payload, uint32_t len);

/**
 * @brief Memaksa sisa buffer masuk ke memori flash dan mengunci berkas secara permanen
 * 
 * @param f Pointer berkas PCAP yang akan ditutup
 */
void pcap_close_file(FILE* f);

#ifdef __cplusplus
}
#endif

#endif // PCAP_WRITER_H
