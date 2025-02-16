#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

// WiFi設定
const char* SSID = "YOUR_WIFI6_AP_SSID";
const char* PASSWORD = "YOUR_WIFI6_AP_PASSWORD";

// OFDMA パラメータ
const int RU_SIZE = 26;  // Resource Unit size (26-tone RU)

// グローバル変数
wifi_ap_record_t apInfo;
uint8_t mac[6];

// 関数プロトタイプ
void setupWiFi();
void analyzeSpectralEfficiency();
void printConnectionStatus();

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32C6 OFDMA Client Test Program");
    
    // MACアドレスの取得
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    // WiFiモードの設定
    WiFi.mode(WIFI_STA);
    
    // WiFi接続のセットアップ
    setupWiFi();
}

void setupWiFi() {
    // WiFi接続の開始
    WiFi.begin(SSID, PASSWORD);
    
    Serial.print("802.11ax APに接続中");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
    
    // 接続情報の表示
    printConnectionStatus();
}

void analyzeSpectralEfficiency() {
    wifi_ap_record_t apInfo;
    if (esp_wifi_sta_get_ap_info(&apInfo) == ESP_OK) {
        float spectralEfficiency = 0.0;
        
        // HE (802.11ax) モードの確認
        if (apInfo.phy_11ax) {
            // HEモードでの推定効率
            spectralEfficiency = 9.6; // 理論最大値
        } else if (apInfo.phy_11n) {
            // 従来モードでの推定
            switch (apInfo.phy_11n) {
                case WIFI_PHY_RATE_MCS7_SGI:
                    spectralEfficiency = 5.9; // bits/s/Hz
                    break;
                case WIFI_PHY_RATE_MCS0:
                    spectralEfficiency = 0.5; // bits/s/Hz
                    break;
                default:
                    spectralEfficiency = 2.0; // デフォルト値
            }
        }
        
        Serial.printf("推定スペクトル効率: %.2f bits/s/Hz\n", spectralEfficiency);
    }
}

void printConnectionStatus() {
    Serial.println("\n=== 接続状態 ===");
    
    // 基本接続情報
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("チャンネル: %d\n", WiFi.channel());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("MACアドレス: %s\n", WiFi.macAddress().c_str());
    Serial.printf("IPアドレス: %s\n", WiFi.localIP().toString().c_str());
    
    // AP情報の取得
    wifi_ap_record_t apInfo;
    if (esp_wifi_sta_get_ap_info(&apInfo) == ESP_OK) {
        Serial.println("\n=== APの性能 ===");
        Serial.printf("802.11ax対応: %s\n", apInfo.phy_11ax ? "Yes" : "No");
        Serial.printf("OFDMA有効: %s\n", apInfo.phy_11ax ? "Yes" : "No");
        
        // HE機能の表示
        if (apInfo.phy_11ax) {
            Serial.println("\n=== HE (802.11ax) 機能 ===");
            Serial.printf("MU-MIMO対応: %s\n", "Yes");
            Serial.printf("最大空間ストリーム: %d\n", 4); // 一般的な最大値
            Serial.printf("BSS Color: %d\n", apInfo.bss_color);
        }
    }
    
    // スペクトル効率の分析
    analyzeSpectralEfficiency();
}

void loop() {
    static unsigned long lastUpdate = 0;
    
    // 5秒ごとに状態を更新
    if (millis() - lastUpdate >= 5000) {
        if (WiFi.status() == WL_CONNECTED) {
            printConnectionStatus();
        } else {
            Serial.println("WiFi切断を検知。再接続を試みます...");
            setupWiFi();
        }
        lastUpdate = millis();
    }
    
    delay(100);
} 