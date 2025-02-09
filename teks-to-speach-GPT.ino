#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"

// ==== Definisi Pin I2S untuk MAX98357A ====
#define I2S_DOUT 25  
#define I2S_BCLK 27  
#define I2S_LRC  26  

Audio audio;  

// ==== Konfigurasi WiFi ====
const char* ssid     = "OUTCOM";
const char* password = "12345678";

// ==== OpenAI API ====
const char* openAiToken = "";  
const char* openAiModel = "gpt-4-turbo";  

// ==== Fungsi untuk Memanggil OpenAI API ====
String getOpenAiResponse(String query) {
  String responseText = "";
  HTTPClient http;
  
  // Endpoint OpenAI API
  String url = "https://api.openai.com/v1/chat/completions";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(openAiToken));

  // Payload request JSON
  String payload = "{\"model\": \"" + String(openAiModel) + "\", \"messages\": [{\"role\": \"user\", \"content\": \"" + query + "\"}], \"max_tokens\": 100}";
  
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    String resp = http.getString();
    
    // Gunakan buffer JSON yang cukup besar untuk mem-parsing respons
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, resp);
    if (!error) {
      responseText = doc["choices"][0]["message"]["content"].as<String>();
      responseText.trim();
    } else {
      Serial.println("Gagal parse JSON dari OpenAI API.");
    }
  } else {
    Serial.printf("Error HTTP OpenAI: %s\n", http.errorToString(httpCode).c_str());
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
    
    String openAiResponse = getOpenAiResponse(query);
    if (openAiResponse == "") {
      Serial.println("Gagal mendapatkan respon dari OpenAI API.");
      return;
    }
    
    Serial.println("Respon OpenAI:");
    Serial.println(openAiResponse);
    
    // Konversi teks respon menjadi suara
    Serial.println("Memainkan respon...");
    audio.connecttospeech(openAiResponse.c_str(), "en");
  }
}

// Callback opsional untuk menampilkan info dari Audio library
void audio_info(const char *info) {
  Serial.print("audio_info: ");
  Serial.println(info);
}
