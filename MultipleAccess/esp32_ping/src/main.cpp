#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include "ping/ping_sock.h"

// Core definitions for ESP32-C6
#define CORE_0 0  // Protocol CPU (160MHz)
#define CORE_1 1  // ULP Core (20MHz)

// WiFi credentials
const char* ssid = "ZZZ_G";
const char* password = "@Aa2890424";

// Ping target
const char* remote_host = "8.8.8.8";  // Google DNS server as default target

// LED pin for status indication
const int STATUS_LED = D6;  // Built-in LED on Seeed XIAO ESP32C6

// Pin definitions
const int ANTENNA_SEL = 14;    // GPIO14 for antenna selection

// Ping statistics
struct PingStats {
    uint32_t transmitted = 0;
    uint32_t received = 0;
    float min_time = UINT32_MAX;
    float max_time = 0;
    float avg_time = 0;
    float m2 = 0;  // For calculating variance using Welford's online algorithm
} ping_stats;

// Antenna selection
typedef enum {
    ANTENNA_INTERNAL = 0,  // Internal PCB antenna (GPIO14 = LOW)
    ANTENNA_EXTERNAL = 1   // External antenna (GPIO14 = HIGH)
} antenna_select_t;

void setAntenna(antenna_select_t ant) {
    pinMode(ANTENNA_SEL, OUTPUT);
    if (ant == ANTENNA_INTERNAL) {
        digitalWrite(ANTENNA_SEL, LOW);
        Serial.println("Selected internal PCB antenna");
    } else {
        digitalWrite(ANTENNA_SEL, HIGH);
        Serial.println("Selected external antenna");
    }
}

void updateStats(uint32_t time) {
    ping_stats.transmitted++;
    ping_stats.received++;
    
    float time_ms = time / 1000.0f;  // Convert to milliseconds
    
    // Sanity check for valid RTT
    if (time_ms < 0 || time_ms > 1000) {  // Ignore RTTs > 1 second
        return;
    }
    
    // Update min/max
    ping_stats.min_time = min(ping_stats.min_time, time_ms);
    ping_stats.max_time = max(ping_stats.max_time, time_ms);
    
    // Update average and M2 using Welford's online algorithm
    float delta = time_ms - ping_stats.avg_time;
    ping_stats.avg_time += delta / ping_stats.received;
    float delta2 = time_ms - ping_stats.avg_time;
    ping_stats.m2 += delta * delta2;
}

void printPingStats() {
    double packet_loss = 100.0 * (ping_stats.transmitted - ping_stats.received) / ping_stats.transmitted;
    double stddev = ping_stats.received > 1 ? sqrt(ping_stats.m2 / (ping_stats.received - 1)) : 0;
    
    Serial.printf("\n--- Ping statistics ---\n");
    Serial.printf("%lu packets transmitted, %lu received, %.2f%% packet loss\n",
                 ping_stats.transmitted,
                 ping_stats.received,
                 packet_loss);
    Serial.printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
                 ping_stats.min_time,
                 ping_stats.avg_time,
                 ping_stats.max_time,
                 stddev);
}

// Function to get WiFi protocol string
const char* getWiFiProtocolString(uint8_t mode) {
    if (mode & WIFI_PROTOCOL_11AX) return "802.11ax (WiFi 6)";
    if (mode & WIFI_PROTOCOL_11N) return "802.11n";
    if (mode & WIFI_PROTOCOL_11G) return "802.11g";
    if (mode & WIFI_PROTOCOL_11B) return "802.11b";
    return "Unknown";
}

// Function to print WiFi details
void printWiFiDetails() {
    Serial.println("\n=== Network Information ===");
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Subnet Mask: %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("Gateway IP: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("DNS Server: %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
    Serial.printf("Channel: %d\n", WiFi.channel());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    
    // Get current WiFi protocol mode
    uint8_t protocol_mode;
    esp_wifi_get_protocol(WIFI_IF_STA, &protocol_mode);
    Serial.printf("Protocol Mode: %s\n", getWiFiProtocolString(protocol_mode));
    
    // Get detailed connection info
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        Serial.printf("Connected AP Protocol: %s\n", 
                     getWiFiProtocolString(ap_info.phy_11ax));
        Serial.printf("AP Signal Strength: %d dBm\n", ap_info.rssi);
        Serial.printf("AP Channel: %d\n", ap_info.primary);
        Serial.printf("AP Bandwidth: %d MHz\n", ap_info.phy_11ax ? 80 : 20);
    }
    
    Serial.println("=========================\n");
}

// Ping configuration
static const esp_ping_config_t ping_config = {
    .count = 1,                // Number of pings to send
    .interval_ms = 0,         // Interval between pings
    .timeout_ms = 1000,       // Timeout for each ping
    .data_size = 32,          // Size of ping data
    .tos = 0,                 // Type of Service
    .target_addr = {0},       // Will be filled in setup
    .task_stack_size = 4096,  // Increased stack size
    .task_prio = 5,           // Increased priority (1-25, higher = more priority)
};

// Ping configuration for rapid ping
static const esp_ping_config_t rapid_ping_config = {
    .count = 1,
    .interval_ms = 0,
    .timeout_ms = 50,         // Reduced timeout for faster response
    .data_size = 32,
    .tos = 0,
    .target_addr = {0},
    .task_stack_size = 4096,
    .task_prio = 18,          // Higher priority (1-25)
};

// Variables for rapid ping
bool rapid_ping_mode = false;
unsigned long rapid_ping_start = 0;
const unsigned long RAPID_PING_DURATION = 3000;  // 3 seconds
const unsigned long RAPID_PING_INTERVAL = 20;    // 20ms interval

// Ping callback results
static void ping_results(esp_ping_handle_t hdl, void *args) {
    uint32_t resp_time;
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &resp_time, sizeof(resp_time));
    updateStats(resp_time);
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
}

esp_ping_handle_t ping_handle = nullptr;

// Task handle for ping task
TaskHandle_t pingTaskHandle = nullptr;

// Ping task function
void pingTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20); // 20ms interval

    while (true) {
        if (rapid_ping_mode) {
            esp_ping_start(ping_handle);
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
        } else {
            vTaskDelay(1);
        }
    }
}

void setup_ping() {
    if (ping_handle != nullptr) {
        esp_ping_delete_session(ping_handle);
    }

    esp_ping_config_t config = ping_config;
    ip_addr_t target_addr;
    ipaddr_aton(remote_host, &target_addr);
    config.target_addr = target_addr;

    esp_ping_callbacks_t cbs;
    cbs.on_ping_success = ping_results;
    cbs.on_ping_timeout = nullptr;
    cbs.on_ping_end = nullptr;
    cbs.cb_args = nullptr;

    esp_ping_new_session(&config, &cbs, &ping_handle);
}

void setup_rapid_ping() {
    if (ping_handle != nullptr) {
        esp_ping_delete_session(ping_handle);
    }

    // Reset statistics
    ping_stats = PingStats();

    esp_ping_config_t config = rapid_ping_config;
    ip_addr_t target_addr;
    ipaddr_aton(WiFi.gatewayIP().toString().c_str(), &target_addr);
    config.target_addr = target_addr;

    esp_ping_callbacks_t cbs;
    cbs.on_ping_success = ping_results;
    cbs.on_ping_timeout = [](esp_ping_handle_t hdl, void *args) {
        ping_stats.transmitted++;
    };
    cbs.on_ping_end = nullptr;
    cbs.cb_args = nullptr;

    esp_ping_new_session(&config, &cbs, &ping_handle);

    // Create ping task on Core 0 with higher priority
    if (pingTaskHandle == nullptr) {
        xTaskCreatePinnedToCore(
            pingTask,
            "PingTask",
            4096,
            NULL,
            18,           // Higher priority (same as config)
            &pingTaskHandle,
            CORE_0
        );
    }
}

// WiFi event handler
void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("WiFi client started");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Connected to WiFi");
            digitalWrite(STATUS_LED, HIGH);
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("Disconnected from WiFi");
            digitalWrite(STATUS_LED, LOW);
            WiFi.begin(ssid, password);
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            printWiFiDetails();
            break;
        default:
            break;
    }
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial) delay(100);
    Serial.println("\nESP32-C6 WiFi 6 Ping Client");

    // Initialize LED
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);

    // Select internal antenna by default
    setAntenna(ANTENNA_INTERNAL);

    // Set WiFi mode and event handler
    WiFi.mode(WIFI_STA);
    WiFi.onEvent(WiFiEvent);

    // Disable power save mode
    esp_wifi_set_ps(WIFI_PS_NONE); //Ping RTT 10ms if WIFI_PS_NONE , else 120ms

    // Configure WiFi 6 (802.11ax) settings with optimized parameters
    wifi_config_t wifi_config = {};
    esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    wifi_config.sta.listen_interval = 1;  // Minimum listen interval
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Maximum TX power

    // Enable WiFi 6 features with optimized protocol settings
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | 
                                      WIFI_PROTOCOL_11G | 
                                      WIFI_PROTOCOL_11N | 
                                      WIFI_PROTOCOL_11AX);

    // Start WiFi connection
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'p' && WiFi.status() == WL_CONNECTED) {
            Serial.printf("\nPINGING %s:\n", WiFi.gatewayIP().toString().c_str());
            rapid_ping_mode = true;
            rapid_ping_start = millis();
            setup_rapid_ping();
        }
        else if (c == 'i') {  // Switch to internal antenna
            setAntenna(ANTENNA_INTERNAL);
        }
        else if (c == 'e') {  // Switch to external antenna
            setAntenna(ANTENNA_EXTERNAL);
        }
    }

    if (rapid_ping_mode) {
        unsigned long current_time = millis();
        
        if (current_time - rapid_ping_start >= RAPID_PING_DURATION) {
            rapid_ping_mode = false;
            if (pingTaskHandle != nullptr) {
                vTaskDelete(pingTaskHandle);
                pingTaskHandle = nullptr;
            }
            printPingStats();
            return;
        }
    }
} 