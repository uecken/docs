#include <Arduino.h>
#include <WiFi.h>

// WiFi設定
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// WiFi接続の品質を監視する変数
int rssi = 0;
uint8_t currentChannel = 0;
wifi_phy_mode_t phyMode;

void printWiFiStatus() {
    rssi = WiFi.RSSI();
    currentChannel = WiFi.channel();
    phyMode = wifi_get_phy_mode();
    
    Serial.println("\n=== WiFi状態 ===");
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("信号強度(RSSI): %d dBm\n", rssi);
    Serial.printf("チャンネル: %d\n", currentChannel);
    Serial.printf("MACアドレス: %s\n", WiFi.macAddress().c_str());
    Serial.printf("IPアドレス: %s\n", WiFi.localIP().toString().c_str());
    
    // PHYモードの表示
    Serial.print("PHYモード: ");
    switch(phyMode) {
        case WIFI_PHY_MODE_11B:
            Serial.println("802.11b (DSSS)");
            break;
        case WIFI_PHY_MODE_11G:
            Serial.println("802.11g");
            break;
        case WIFI_PHY_MODE_11N:
            Serial.println("802.11n");
            break;
        case WIFI_PHY_MODE_LR:
            Serial.println("Long Range");
            break;
        default:
            Serial.println("Unknown");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32 WiFiテストプログラム - 多元接続テスト");
    
    // WiFi接続開始
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("WiFiに接続中");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi接続完了!");
    
    // 初期状態の表示
    printWiFiStatus();
}

void loop() {
    static unsigned long lastCheck = 0;
    
    // 5秒ごとにWiFi状態をチェック
    if (millis() - lastCheck >= 5000) {
        printWiFiStatus();
        lastCheck = millis();
    }
    
    // WiFi切断時の再接続処理
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi切断を検知。再接続を試みます...");
        WiFi.disconnect();
        WiFi.begin(ssid, password);
    }
    
    delay(100);
} 