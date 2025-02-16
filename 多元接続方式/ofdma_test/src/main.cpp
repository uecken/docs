#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

// WiFi設定
const char* SSID = "YOUR_SSID";
const char* PASSWORD = "YOUR_PASSWORD";

// OFDMA パラメータ
const int RU_SIZE = 26;  // Resource Unit size (26-tone RU)
const int MAX_USERS = 4; // 最大同時接続ユーザー数

// グローバル変数
wifi_sta_list_t stationList;
wifi_ap_record_t apInfo;
uint8_t mac[6];

// 関数プロトタイプ
void setupAP();
void monitorConnections();
void analyzeSpectralEfficiency();
void printOFDMAStatus();

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32 OFDMA Test Program");
    
    // MACアドレスの取得
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    // WiFiモードの設定
    WiFi.mode(WIFI_AP_STA);
    
    // APモードのセットアップ
    setupAP();
    
    // STAモードの接続
    WiFi.begin(SSID, PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi Connected!");
}

void setupAP() {
    // APモードの設定
    String apSSID = "ESP32_OFDMA_" + String(mac[4], HEX) + String(mac[5], HEX);
    
    // OFDMA有効化のための設定
    wifi_config_t conf;
    memset(&conf, 0, sizeof(conf));
    strcpy((char*)conf.ap.ssid, apSSID.c_str());
    conf.ap.ssid_len = apSSID.length();
    conf.ap.max_connection = MAX_USERS;
    conf.ap.authmode = WIFI_AUTH_WPA2_PSK;
    
    // HE (High Efficiency) モードの有効化
    conf.ap.he_dcm_enable = 1;
    conf.ap.he_su_beamformer = 1;
    conf.ap.he_su_beamformee = 1;
    conf.ap.he_mu_beamformer = 1;
    
    esp_wifi_set_config(WIFI_IF_AP, &conf);
    
    WiFi.softAP(apSSID.c_str(), "password123");
}

void analyzeSpectralEfficiency() {
    wifi_ap_record_t apInfo;
    if (esp_wifi_sta_get_ap_info(&apInfo) == ESP_OK) {
        float spectralEfficiency = 0.0;
        
        // 変調方式に基づく推定
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
        
        Serial.printf("推定スペクトル効率: %.2f bits/s/Hz\n", spectralEfficiency);
    }
}

void printOFDMAStatus() {
    Serial.println("\n=== OFDMA状態 ===");
    
    // 接続デバイス数の取得
    wifi_sta_list_t stations;
    esp_wifi_ap_get_sta_list(&stations);
    
    Serial.printf("接続デバイス数: %d\n", stations.num);
    
    // リソースユニットの使用状況
    int totalRUs = RU_SIZE * stations.num;
    Serial.printf("使用中RU数: %d\n", totalRUs);
    
    // 各デバイスの情報表示
    for (int i = 0; i < stations.num; i++) {
        Serial.printf("デバイス %d:\n", i + 1);
        Serial.printf("  MACアドレス: %02X:%02X:%02X:%02X:%02X:%02X\n",
            stations.sta[i].mac[0], stations.sta[i].mac[1],
            stations.sta[i].mac[2], stations.sta[i].mac[3],
            stations.sta[i].mac[4], stations.sta[i].mac[5]);
        Serial.printf("  RSSI: %d dBm\n", stations.sta[i].rssi);
    }
    
    // スペクトル効率の分析
    analyzeSpectralEfficiency();
}

void loop() {
    static unsigned long lastUpdate = 0;
    
    // 5秒ごとに状態を更新
    if (millis() - lastUpdate >= 5000) {
        printOFDMAStatus();
        lastUpdate = millis();
    }
    
    delay(100);
} 