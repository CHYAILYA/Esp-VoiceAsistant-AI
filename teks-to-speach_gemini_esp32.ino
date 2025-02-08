#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"

// ==== Definisi Pin I2S untuk MAX98357A ====
// Sesuaikan dengan wiring hardware Anda
#define I2S_DOUT 25  // Data Out
#define I2S_BCLK 27  // Bit Clock
#define I2S_LRC  26  // Left/Right Clock

Audio audio;  

// ==== Konfigurasi WiFi ====
const char* ssid     = "OUTCOM";
const char* password = "12345678";

// ==== API Gemini ====
const char* geminiToken   = "your-api-key";  // Hanya API key Gemini yang diperlukan
const int geminiMaxTokens = 100;  // Jumlah maksimum token keluaran (sesuaikan jika perlu)

// ==== Fungsi untuk Memanggil Gemini API ====
String getGeminiResponse(String query) {
  String responseText = "";
  HTTPClient http;
  
  // Endpoint Gemini API (pastikan endpoint dan model sesuai dengan dokumentasi terbaru)
  String url = String("https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=") + geminiToken;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"contents\": [{\"parts\":[{\"text\":\"" + query + "\"}]}], \"generationConfig\": {\"maxOutputTokens\": " + String(geminiMaxTokens) + "}}";
  
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    String resp = http.getString();
    
    // Gunakan buffer JSON yang cukup besar untuk mem-parsing respons
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, resp);
    if (!error) {
      responseText = doc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
      responseText.trim();
    } else {
      Serial.println("Gagal parse JSON dari Gemini API.");
    }
  } else {
    Serial.printf("Error HTTP Gemini: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
  return responseText;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Inisialisasi Audio library dengan konfigurasi I2S untuk MAX98357A
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100);
  
  Serial.println("\nMasukkan pertanyaan Anda melalui Serial Monitor:");
}

void loop() {
  audio.loop();

  if (Serial.available() > 0) {
    String query = Serial.readStringUntil('\n');
    query.trim();
    
    if (query.length() == 0) {
      Serial.println("Input kosong, silakan masukkan pertanyaan yang valid.");
      return;
    }
    
    Serial.print("Anda: ");
    Serial.println(query);
    
    String geminiResponse = getGeminiResponse(query);
    if (geminiResponse == "") {
      Serial.println("Gagal mendapatkan respon dari Gemini API.");
      return;
    }
    
    Serial.println("Respon Gemini:");
    Serial.println(geminiResponse);
    
    // 2. Gunakan Audio library untuk mengubah teks respon menjadi suara
    // Perbarui: Konversi geminiResponse menjadi const char* dengan .c_str()
    Serial.println("Memainkan respon...");
    audio.connecttospeech(geminiResponse.c_str(), "en");
  }
}

// Callback opsional untuk menampilkan info dari Audio library
void audio_info(const char *info) {
  Serial.print("audio_info: ");
  Serial.println(info);
}
