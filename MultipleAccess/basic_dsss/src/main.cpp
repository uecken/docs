#include <Arduino.h>
#include <WiFi.h>
#include <driver/adc.h>

// DSSS simulation parameters
const int CHIP_SEQUENCE_LENGTH = 11;  // 11-bit Barker code
const int8_t barkerCode[11] = {+1, -1, +1, +1, -1, +1, +1, +1, -1, -1, -1};
const int SAMPLE_RATE = 1000;  // Hz
const int DATA_RATE = 100;     // Hz

// ADC configuration
const adc1_channel_t NOISE_CHANNEL = ADC1_CHANNEL_0;  // GPIO36
const int OUTPUT_PIN = 25;  // DAC output pin

// Global variables
int dataStream[1000];  // Store raw data
int spreadData[11000]; // Store spread spectrum data

// Function prototypes
void generateRandomData();
void applyDSSS();
void measureSignalQuality();
void transmitData();

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32 DSSS Simulation and Test Program");
    
    // ADCの設定
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(NOISE_CHANNEL, ADC_ATTEN_DB_11);
    
    // DACの設定
    dacWrite(OUTPUT_PIN, 0);
}

void generateRandomData() {
    for (int i = 0; i < 1000; i++) {
        dataStream[i] = (random(2) * 2) - 1;  // Generate -1 or 1
    }
}

void applyDSSS() {
    int spreadIndex = 0;
    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < CHIP_SEQUENCE_LENGTH; j++) {
            spreadData[spreadIndex++] = dataStream[i] * barkerCode[j];
        }
    }
}

void measureSignalQuality() {
    float snr = 0;
    float signal = 0;
    float noise = 0;
    
    // 信号電力の測定
    for (int i = 0; i < 1000; i++) {
        signal += sq(dataStream[i]);
    }
    signal /= 1000;
    
    // ノイズ電力の測定
    for (int i = 0; i < 1000; i++) {
        int noiseReading = adc1_get_raw(NOISE_CHANNEL);
        noise += sq(noiseReading - 2048);  // 2048 is the midpoint for 12-bit ADC
    }
    noise /= 1000;
    
    // SNR計算 (dB)
    snr = 10 * log10(signal / noise);
    
    Serial.println("\n=== 信号品質測定 ===");
    Serial.printf("信号電力: %.2f\n", signal);
    Serial.printf("ノイズ電力: %.2f\n", noise);
    Serial.printf("SNR: %.2f dB\n", snr);
}

void transmitData() {
    static int index = 0;
    
    // DACを使用してスペクトラム拡散信号を出力
    int dacValue = map(spreadData[index], -1, 1, 0, 255);
    dacWrite(OUTPUT_PIN, dacValue);
    
    index = (index + 1) % 11000;
}

void loop() {
    static unsigned long lastUpdate = 0;
    
    // 1秒ごとに新しいデータを生成
    if (millis() - lastUpdate >= 1000) {
        generateRandomData();
        applyDSSS();
        measureSignalQuality();
        lastUpdate = millis();
    }
    
    // 連続的にデータを送信
    transmitData();
    delayMicroseconds(100);  // 送信レート制御
} 