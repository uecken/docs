#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_mac.h>  // ESP32-C6用のMACアドレス取得ヘッダー
#include <esp_wifi_he.h>  // HE (802.11ax) 機能用
#include <esp_netif.h>

// WiFi設定
const char* ssid = "ZZZ_G";
const char* password = "@Aa2890424";

// スループットテスト設定
const uint16_t SERVER_PORT = 5001;
const int TEST_DURATION = 10; // テスト時間（秒）
const int BUFFER_SIZE = 1460; // TCP MSS size
uint8_t buffer[BUFFER_SIZE];
uint8_t rx_buffer[BUFFER_SIZE];

// グローバル変数
wifi_ap_record_t apInfo;
uint8_t mac[6];
WiFiClient client;
bool statusPrinted = false;

// 関数プロトタイプ
void setupWiFi();
void analyzeSpectralEfficiency();
void printConnectionStatus();
void runUplinkTest(const char* server_ip);
void runDownlinkTest(const char* server_ip);
void printDetailedWiFiInfo();
String getMCSDescription(uint8_t mcs);
String getBandwidthString(wifi_bandwidth_t bw);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\nESP32C6 Throughput Test");
    
    // MACアドレスの取得
    WiFi.macAddress(mac);
    
    // WiFiモードの設定
    WiFi.mode(WIFI_STA);
    
    // WiFi接続のセットアップ
    setupWiFi();
    
    // 初回の接続情報表示
    printConnectionStatus();
    statusPrinted = true;
}

void setupWiFi() {
    // WiFi接続の開始
    WiFi.begin(ssid, password);
    
    Serial.print("WiFiに接続中...");
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 20) {
        delay(500);
        Serial.print(".");
        retryCount++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.printf("IPアドレス: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nWiFi接続に失敗しました");
        ESP.restart();
    }
}

void runUplinkTest(const char* server_ip) {
    Serial.printf("\n=== アップリンクスループットテスト開始 ===\n");
    Serial.printf("サーバー: %s:%d\n", server_ip, SERVER_PORT);
    
    if (!client.connect(server_ip, SERVER_PORT)) {
        Serial.println("サーバーへの接続に失敗しました");
        return;
    }
    
    uint32_t startTime = millis();
    uint32_t totalBytes = 0;
    uint32_t lastPrint = startTime;
    uint32_t lastActivity = startTime;
    
    // テストループ
    while (millis() - startTime < TEST_DURATION * 1000) {
        if (client.connected()) {
            int sent = client.write(buffer, BUFFER_SIZE);
            if (sent > 0) {
                totalBytes += sent;
                lastActivity = millis();
            }
            
            // 1秒ごとに中間結果を表示
            if (millis() - lastPrint >= 1000) {
                float currentMbps = (totalBytes * 8.0) / (millis() - startTime) / 1000.0;
                Serial.printf("現在のULスループット: %.2f Mbps\n", currentMbps);
                lastPrint = millis();
            }
            
            // 5秒以上データ送信がない場合は中断
            if (millis() - lastActivity >= 5000) {
                Serial.println("データ送信のタイムアウト");
                break;
            }
        } else {
            Serial.println("接続が切断されました");
            break;
        }
    }
    
    // 最終結果の計算と表示
    float duration = (millis() - startTime) / 1000.0;
    float totalMbps = (totalBytes * 8.0) / duration / 1000000.0;
    
    Serial.println("\n=== アップリンクテスト結果 ===");
    Serial.printf("総転送量: %.2f MB\n", totalBytes / 1000000.0);
    Serial.printf("テスト時間: %.2f 秒\n", duration);
    Serial.printf("平均ULスループット: %.2f Mbps\n", totalMbps);
    
    client.stop();
}

void runDownlinkTest(const char* server_ip) {
    Serial.printf("\n=== ダウンリンクスループットテスト開始 ===\n");
    Serial.printf("サーバー: %s:%d\n", server_ip, SERVER_PORT);
    
    if (!client.connect(server_ip, SERVER_PORT)) {
        Serial.println("サーバーへの接続に失敗しました");
        return;
    }
    
    // iPerf3プロトコルの初期化
    // テストクッキーの生成（ランダムな32ビット値）
    uint32_t cookie = esp_random();  // より良いランダム値の生成
    
    // ヘッダー情報の構築
    char header[256];
    snprintf(header, sizeof(header),
        "{\"client_version\":\"3.1.3\","
        "\"test_start\":{"
        "\"protocol\":\"TCP\","
        "\"reverse\":1,"
        "\"omit\":0,"
        "\"parallel\":1,"
        "\"time\":%d,"
        "\"bytes\":0,"
        "\"blocks\":0,"
        "\"len\":%d,"
        "\"cookie\":\"%x\","
        "\"num_streams\":1,"
        "\"throughput_units\":\"bps\","
        "\"congestion\":\"cubic\""
        "}}\n",
        TEST_DURATION, BUFFER_SIZE, cookie);
    
    // ヘッダー長を4バイトのネットワークバイトオーダーで送信
    uint32_t headerLen = strlen(header);
    uint8_t lenBytes[4];
    lenBytes[0] = (headerLen >> 24) & 0xFF;
    lenBytes[1] = (headerLen >> 16) & 0xFF;
    lenBytes[2] = (headerLen >> 8) & 0xFF;
    lenBytes[3] = headerLen & 0xFF;
    
    // デバッグ情報の表示
    Serial.printf("ヘッダー長: %d バイト\n", headerLen);
    Serial.printf("送信するヘッダー: %s", header);  // 改行は既にヘッダーに含まれている
    
    // ヘッダー長とヘッダーを送信
    if (client.write(lenBytes, 4) != 4) {
        Serial.println("ヘッダー長の送信に失敗");
        client.stop();
        return;
    }
    
    if (client.write((uint8_t*)header, headerLen) != headerLen) {
        Serial.println("ヘッダーの送信に失敗");
        client.stop();
        return;
    }
    
    client.flush();
    
    // サーバーからの応答を待つ
    uint32_t waitStart = millis();
    bool serverReady = false;
    
    while (millis() - waitStart < 5000) {  // 5秒間待つ
        if (client.available()) {
            // サーバーからの応答を読み取る
            String response = client.readStringUntil('\n');
            Serial.printf("サーバーからの応答: %s\n", response.c_str());
            if (response.indexOf("test_start") > 0) {
                serverReady = true;
                break;
            }
        }
        delay(10);
    }
    
    if (!serverReady) {
        Serial.println("サーバーからの応答がありませんでした");
        client.stop();
        return;
    }
    
    Serial.println("テスト開始");
    uint32_t startTime = millis();
    uint32_t totalBytes = 0;
    uint32_t lastPrint = startTime;
    uint32_t lastActivity = startTime;
    
    // テストループ
    while (millis() - startTime < TEST_DURATION * 1000) {
        if (client.connected()) {
            int available = client.available();
            if (available > 0) {
                int toRead = min(available, BUFFER_SIZE);
                int received = client.read(rx_buffer, toRead);
                if (received > 0) {
                    totalBytes += received;
                    lastActivity = millis();
                    
                    // 1秒ごとに中間結果を表示
                    if (millis() - lastPrint >= 1000) {
                        float currentMbps = (totalBytes * 8.0) / (millis() - startTime) / 1000.0;
                        Serial.printf("現在のDLスループット: %.2f Mbps (受信: %d bytes)\n", 
                            currentMbps, received);
                        lastPrint = millis();
                    }
                }
            }
            
            // 3秒以上データ受信がない場合は中断
            if (millis() - lastActivity >= 3000) {
                Serial.println("データ受信のタイムアウト");
                break;
            }
        } else {
            Serial.println("接続が切断されました");
            break;
        }
        
        delay(1);
    }
    
    // 最終結果の計算と表示
    float duration = (millis() - startTime) / 1000.0;
    float totalMbps = (totalBytes * 8.0) / duration / 1000000.0;
    
    Serial.println("\n=== ダウンリンクテスト結果 ===");
    Serial.printf("総受信量: %.2f MB\n", totalBytes / 1000000.0);
    Serial.printf("テスト時間: %.2f 秒\n", duration);
    Serial.printf("平均DLスループット: %.2f Mbps\n", totalMbps);
    
    client.stop();
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
            spectralEfficiency = 2.0; // デフォルト値
        }
        
        Serial.printf("推定スペクトル効率: %.2f bits/s/Hz\n", spectralEfficiency);
    }
}

void printDetailedWiFiInfo() {
    wifi_ap_record_t ap_info;
    wifi_bandwidth_t bw;
    
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK &&
        esp_wifi_get_bandwidth(WIFI_IF_STA, &bw) == ESP_OK) {
        
        Serial.println("\n=== 詳細なWiFi情報 ===");
        Serial.printf("BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
            ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
            ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
        Serial.printf("チャンネル: %d\n", ap_info.primary);
        Serial.printf("RSSI: %d dBm\n", ap_info.rssi);

        // PHYモード情報
        Serial.println("\n=== PHYモード ===");
        if (ap_info.phy_11ax) {
            Serial.printf("PHYモード: HE (%s)\n", getBandwidthString(bw).c_str());
            Serial.println("OFDMA: 有効");
            Serial.printf("RU Size: %s\n", 
                bw == WIFI_BW_HT40 ? "52-tone (40MHz)" : "26-tone (20MHz)");
        } else if (ap_info.phy_11n) {
            Serial.printf("PHYモード: HT (%s)\n", getBandwidthString(bw).c_str());
        } else if (ap_info.phy_11g) {
            Serial.println("PHYモード: 11g");
        } else {
            Serial.println("PHYモード: 11b");
        }
        
        // 基本的なリンク情報
        Serial.println("\n=== リンク情報 ===");
        Serial.printf("空間ストリーム数: 1\n");
        Serial.printf("Guard Interval: %s\n", 
            ap_info.phy_11ax ? "0.8µs (HE)" : "0.4µs (Legacy)");
    } else {
        Serial.println("WiFi情報の取得に失敗しました");
    }
}

String getMCSDescription(uint8_t mcs) {
    char desc[64];
    snprintf(desc, sizeof(desc), "MCS%d (最大 %d Mbps)", 
             mcs, 
             mcs <= 7 ? (mcs + 1) * 86 : 0);  // 概算値
    return String(desc);
}

String getBandwidthString(wifi_bandwidth_t bw) {
    switch(bw) {
        case WIFI_BW_HT20:
            return "20MHz";
        case WIFI_BW_HT40:
            return "40MHz";
        default:
            return "Unknown";
    }
}

void printConnectionStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n=== 接続情報 ===");
        Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("チャンネル: %d\n", WiFi.channel());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
        Serial.printf("MACアドレス: %02X:%02X:%02X:%02X:%02X:%02X\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Serial.printf("IPアドレス: %s\n", WiFi.localIP().toString().c_str());
        
        // 詳細なWiFi情報の表示
        printDetailedWiFiInfo();
    } else {
        Serial.println("WiFiに接続されていません");
    }
}

void loop() {
    // WiFi接続が切れた場合の再接続
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi接続が切断されました。再接続を試みます...");
        setupWiFi();
        printConnectionStatus();
    }
    
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'u' || cmd == 'd') {
            Serial.println("\nサーバーIPアドレスを入力してください（例: 192.168.1.100）:");
            while (!Serial.available()) {
                delay(100);
            }
            String serverIP = Serial.readStringUntil('\n');
            serverIP.trim();
            
            if (cmd == 'u') {
                runUplinkTest(serverIP.c_str());
            } else {
                runDownlinkTest(serverIP.c_str());
            }
        }
        else if (cmd == 's') {
            // 手動でステータス表示（詳細情報を含む）
            printConnectionStatus();
        }
        else if (cmd == 'i') {
            // 詳細なWiFi情報のみ表示
            printDetailedWiFiInfo();
        }
    }
    delay(100);
} 