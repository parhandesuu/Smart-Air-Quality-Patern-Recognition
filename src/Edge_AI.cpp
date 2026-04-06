#define BLYNK_TEMPLATE_ID "TMPL6ntiETsbH"
#define BLYNK_TEMPLATE_NAME "Project CaAS 3 Smart AQPR"
#define BLYNK_AUTH_TOKEN "R4GlnEDv-nUWmMyJJ1in-eOWiRLqZMcI"
#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EloquentTinyML.h>
#include <time.h>
#include "sine_model.h" 

// --- KONFIGURASI WIFI ---
char ssid[] = "Galaxy A73 5GCB26";
char pass[] = "dzyy6328";

// --- KONFIGURASI PIN (Sesuai Mappi32/ESP32-S3 kamu) ---
#define MQ2PIN      2   
#define DHTPIN      6   
#define LED_RED     7   
#define LED_YELLOW  21  
#define LED_GREEN   20  
#define I2C_SDA     8   
#define I2C_SCL     9   
#define DHTTYPE     DHT11

// --- KONFIGURASI AI (TFLite) ---
#define NUMBER_OF_INPUTS 3   
#define NUMBER_OF_OUTPUTS 3  
#define TENSOR_ARENA_SIZE 8*1024 

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;
Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

String labels[] = {"Clean", "Stale", "Hazardous"};
int lastStatus = -1; 

void mainSystem() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    int gasRaw = analogRead(MQ2PIN);

    if (isnan(t) || isnan(h)) return;

    // --- 1. NORMALISASI DATA ---
    // AI belajar dengan angka 0-1, maka kita konversi angka mentah sensor ke skala 0-1
    float input[3] = { 
        t / 50.0f,               
        h / 100.0f,              
        (float)gasRaw / 4095.0f  
    };

    // --- 2. PREDIKSI AI DENGAN CUSTOM SENSITIVITY ---
    float output[3] = {0, 0, 0}; 
    ml.predict(input, output); 

    int predictedClass = 0;

    // Logika Sensitivitas:
    if (output[2] >= 0.20) {
        // Jika peluang Hazardous mencapai 20% atau lebih
        predictedClass = 2; 
    } else if (output[1] >= 0.20) {
        // Jika tidak bahaya, tapi peluang Stale mencapai 20% atau lebih
        predictedClass = 1;
    } else {
        // Sisanya masuk ke Clean (Selama Clean >= 40%)
        predictedClass = 0;
    }

    String statusUdara = labels[predictedClass];

    // Debugging Tambahan
    Serial.printf("Peluang -> Clean: %.2f | Stale: %.2f | Bahaya: %.2f\n", output[0], output[1], output[2]);

    // --- 3. KONTROL LED FISIK ---
    digitalWrite(LED_GREEN, (predictedClass == 0));
    digitalWrite(LED_YELLOW, (predictedClass == 1));
    digitalWrite(LED_RED, (predictedClass == 2));

    // Update LCD
    lcd.setCursor(0, 0);
    lcd.print("T:"); lcd.print(t, 1); lcd.print(" G:"); lcd.print(gasRaw); lcd.print("    ");
    lcd.setCursor(0, 1);
    lcd.print("ST:"); lcd.print(statusUdara); lcd.print("        ");

    // --- 4. UPDATE BLYNK ---
    if (WiFi.status() == WL_CONNECTED && Blynk.connected()) {
        Blynk.virtualWrite(V0, t);
        Blynk.virtualWrite(V1, gasRaw);
        Blynk.virtualWrite(V3, h);
        Blynk.virtualWrite(V6, statusUdara);
        
        // Menyalakan LED V2 di Dashboard
        Blynk.virtualWrite(V2, 255); 

        // LED V2 secara Otomatis
        if (predictedClass == 0) {
            Blynk.setProperty(V2, "color", "#0cbe0f"); // Hijau (Blynk Green)
        } else if (predictedClass == 1) {
            Blynk.setProperty(V2, "color", "#ED9D00"); // Kuning/Oranye (Blynk Yellow)
        } else if (predictedClass == 2) {
            Blynk.setProperty(V2, "color", "#D34336"); // Merah (Blynk Red)
        }

        // Notifikasi Log
        if (predictedClass != lastStatus) {
            if (predictedClass == 2) Blynk.logEvent("hazardous_", "WARNING: Kondisi Berbahaya!");
            else Blynk.logEvent("state", "Status: " + statusUdara);
            lastStatus = predictedClass;
        }

        // Update Jam (V4, V5, V7)
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            char tStr[10], dStr[20], dyStr[15];
            strftime(tStr, sizeof(tStr), "%H:%M", &timeinfo);
            strftime(dStr, sizeof(dStr), "%d/%m/%Y", &timeinfo);
            strftime(dyStr, sizeof(dyStr), "%A", &timeinfo);
            Blynk.virtualWrite(V5, tStr); 
            Blynk.virtualWrite(V4, dStr); 
            Blynk.virtualWrite(V7, dyStr); 
        }
    }

    Serial.printf("Prob -> C:%.2f S:%.2f H:%.2f | LED V2 Color Changed!\n", output[0], output[1], output[2]);
}

void setup() {
    Serial.begin(115200);
    
    // Inisialisasi Output
    pinMode(LED_RED, OUTPUT); 
    pinMode(LED_YELLOW, OUTPUT); 
    pinMode(LED_GREEN, OUTPUT);
    
    // Inisialisasi Sensor & LCD
    dht.begin();
    Wire.begin(I2C_SDA, I2C_SCL);
    lcd.init(); 
    lcd.backlight();
    lcd.setCursor(0, 0); lcd.print("Smart AQPR");
    lcd.setCursor(0, 1); lcd.print("Loading AI...");

    // --- STRATEGI WIFI NON-BLOCKING ---
    WiFi.begin(ssid, pass);
    Blynk.config(BLYNK_AUTH_TOKEN);
    configTime(7 * 3600, 0, "pool.ntp.org"); // Set Waktu (WIB)

    // Inisialisasi Otak AI
    if (!ml.begin(model_data)) {
        Serial.println("Model AI Gagal Dimuat!");
        lcd.setCursor(0, 1); lcd.print("AI ERROR!");
        while (1);
    }
    
    lcd.clear();
    timer.setInterval(5000L, mainSystem); // Update setiap 5 detik
}

void loop() {
    // Hanya jalankan Blynk jika WiFi tersambung (agar tidak lag saat offline)
    if (WiFi.status() == WL_CONNECTED) {
        Blynk.run();
    }
    timer.run();
}