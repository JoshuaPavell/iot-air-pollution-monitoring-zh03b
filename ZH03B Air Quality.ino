#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPL6e_Sx5JvD"
#define BLYNK_TEMPLATE_NAME "Polusi Udara"
#define BLYNK_DEVICE_NAME "Polusi Udara"
#define RELAY_PIN 4  // Menggunakan GPIO 4 untuk relay

#include <SD_ZH03B.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "Tjokahn Atas";
char pass[] = "jerome101211";
char auth[] = "pDsbroQm7YczWBFeeNMN7GWcVhkZta9G";

// Deklarasi objek sensor
SoftwareSerial ZHSerial(17, 16);
SD_ZH03B ZH03B(ZHSerial);

void setup() {
    Serial.begin(9600);
    Serial.println();
    delay(1000);
    Serial.println("-- Initializing Sensor...");
    ZH03B.setMode( SD_ZH03B::QA_MODE );

    // Inisialisasi komunikasi serial untuk sensor
    ZHSerial.begin(9600);
    delay(100);

    // Koneksi ke WiFi dan Blynk
    connectWiFi();
    Blynk.begin(auth, ssid, pass);

    // Inisialisasi pin relay sebagai output
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Pastikan relay mati pada awalnya
}

void loop() {
    Blynk.run();
    
    // Membaca data dari sensor
    if (ZH03B.readData()) {
        // Memeriksa apakah nilai PM masuk akal
        if (ZH03B.getPM1_0() > 0 && ZH03B.getPM2_5() > 0 && ZH03B.getPM10_0() > 0) {
            Serial.print("PM1: ");
            float pm1Value = ZH03B.getPM1_0();
            Serial.print(pm1Value);
            Serial.print(" ug/m^3, PM2.5: ");
            float pm25Value = ZH03B.getPM2_5();
            Serial.print(pm25Value);
            Serial.print(" ug/m^3, PM10: ");
            float pm10Value = ZH03B.getPM10_0();
            Serial.print(pm10Value);
            Serial.println(" ug/m^3");
            
            // Menghitung AQI
            int aqi = calculateAQI(pm25Value, pm10Value, pm1Value);
            Serial.print("AQI: ");
            Serial.println(aqi);

            // Mengirimkan nilai PM dan AQI ke Blynk
            Blynk.virtualWrite(V0, pm25Value);
            Blynk.virtualWrite(V1, pm10Value);
            Blynk.virtualWrite(V2, pm1Value);
            Blynk.virtualWrite(V4, aqi);

            // Mengirimkan notifikasi ke Blynk
            if(aqi >= 51 && aqi <= 100){
              Blynk.logEvent("sedang");
              Serial.println("Notifikasi Berhasil Terkirim");
              digitalWrite(RELAY_PIN, LOW);  // Mematikan relay
            }

            if(aqi >= 101 && aqi <= 150){
              Blynk.logEvent("kurang_sehat");
              Serial.println("Notifikasi Berhasil Terkirim");
              digitalWrite(RELAY_PIN, HIGH);  // Mengaktifkan relay
            }

            if(aqi >= 151 && aqi <= 200){
              Blynk.logEvent("tidak_sehat");
              Serial.println("Notifikasi Berhasil Terkirim");
              digitalWrite(RELAY_PIN, HIGH);  // Mengaktifkan relay
            }

            if(aqi >= 201 && aqi <= 300){
              Blynk.logEvent("sangat_tidak_sehat");
              Serial.println("Notifikasi Berhasil Terkirim");
              digitalWrite(RELAY_PIN, HIGH);  // Mengaktifkan relay
            }

            if(aqi >= 301){
              Blynk.logEvent("berbahaya");
              Serial.println("Notifikasi Berhasil Terkirim");
              digitalWrite(RELAY_PIN, HIGH);  // Mengaktifkan relay
            }

        } else {
            Serial.println("Error: Sensor memberikan data yang tidak valid");
            // Tindakan pemulihan jika data yang dibaca tidak valid
            delay(5000); // Menunggu 5 detik sebelum mencoba kembali
        }
    } else {
        Serial.println("Error: Gagal membaca data sensor atau checksum error");
        // Tindakan pemulihan jika sensor tidak terhubung atau mengalami error
        ZHSerial.end(); // Mengakhiri komunikasi serial dengan sensor
        delay(1000); // Menunggu 1 detik sebelum mencoba kembali
        ZHSerial.begin(9600); // Menginisialisasi ulang komunikasi serial dengan sensor
        delay(1000); // Menunggu 1 detik sebelum mencoba membaca data dari sensor
    }

    Serial.println("-----------------");
    delay(10000); // Delay 10 detik antara bacaan
}

void connectWiFi() {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, pass);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {                    
        Serial.println("\nWiFi connected");
    } else {
        Serial.println("\nFailed to connect to WiFi");
        // Tindakan pemulihan jika gagal terhubung ke WiFi
    }
}

int calculateAQI(float pm25Value, float pm10Value, float pm1Value) {
    // Array batas bawah dari setiap rentang PM2.5, PM10, dan PM1 untuk setiap kategori AQI
    float pm25Values[] = {0, 12.1, 35.5, 55.5, 150.5, 250.5, 350.5, 500.5};
    float pm10Values[] = {0, 55, 155, 255, 355, 425, 505, 605};
    float pm1Values[] = {0, 12.1, 35.5, 55.5, 150.5, 250.5, 350.5, 500.5};
    
    // Array nilai kategori AQI yang sesuai dengan setiap rentang PM2.5, PM10, dan PM1
    int aqiValues[] = {0, 50, 100, 150, 200, 300, 400, 500};
    
    // Cari dua titik data terdekat untuk PM2.5
    int i = 0;
    while (i < 7 && pm25Value > pm25Values[i + 1]) {
        i++;
    }
    
    // Interpolasi linier untuk PM2.5
    int aqi_pm25 = aqiValues[i] + ((pm25Value - pm25Values[i]) * (aqiValues[i + 1] - aqiValues[i])) / (pm25Values[i + 1] - pm25Values[i]);
    
    // Cari dua titik data terdekat untuk PM10
    i = 0;
    while (i < 7 && pm10Value > pm10Values[i + 1]) {
        i++;
    }
    
    // Interpolasi linier untuk PM10
    int aqi_pm10 = aqiValues[i] + ((pm10Value - pm10Values[i]) * (aqiValues[i + 1] - aqiValues[i])) / (pm10Values[i + 1] - pm10Values[i]);

    // Cari dua titik data terdekat untuk PM1
    i = 0;
    while (i < 7 && pm1Value > pm1Values[i + 1]) {
        i++;
    }
    
    // Interpolasi linier untuk PM1
    int aqi_pm1 = aqiValues[i] + ((pm1Value - pm1Values[i]) * (aqiValues[i + 1] - aqiValues[i])) / (pm1Values[i + 1] - pm1Values[i]);

    // Ambil nilai AQI maksimum dari ketiga nilai AQI (PM2.5, PM10, PM1)
    return max(max(aqi_pm25, aqi_pm10), aqi_pm1);
}

