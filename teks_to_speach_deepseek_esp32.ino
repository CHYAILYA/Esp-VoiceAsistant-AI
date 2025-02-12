#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "Audio.h"

// ==== Konfigurasi Hardware ====
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC  26

Audio audio;

// ==== Konfigurasi Jaringan ====
const char* ssid     = "OUTCOM";
const char* password = "12345678";

// ==== Konfigurasi API ====
const char* hfToken = "tokent";
const int maxTokens = 500;
const int SERIAL_WIDTH = 80;
const int MAX_TTS_LENGTH = 300; // Maksimal karakter untuk TTS

// ==== Prototipe Fungsi ====
String getHFResponse(String query);
void printFormatted(String text);
void cleanTTSText(String &text);
bool playTTS(String text);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Inisialisasi WiFi
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  
  // Inisialisasi Audio
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100); // Volume maksimal
  
  Serial.println("\nMasukkan pertanyaan Anda melalui Serial Monitor:");
}

void loop() {
  audio.loop();

  if (Serial.available() > 0) {
    String query = Serial.readStringUntil('\n');
    query.trim();
    
    if (query.isEmpty()) {
      Serial.println("Input kosong!");
      return;
    }
    
    Serial.print("\nAnda: ");
    Serial.println(query);
    
    String hfResponse = getHFResponse(query);
    cleanTTSText(hfResponse);
    
    if (hfResponse.isEmpty()) {
      Serial.println("Gagal mendapatkan respon");
      return;
    }
    
    Serial.println("\nRespon:");
    printFormatted(hfResponse);

    if(!playTTS(hfResponse)) {
      Serial.println("Gagal memulai text-to-speech");
    }
  }
}

bool playTTS(String text) {
  // Potong teks jika terlalu panjang
  if(text.length() > MAX_TTS_LENGTH) {
    text = text.substring(0, MAX_TTS_LENGTH);
    text += "... (truncated)";
  }
  
  Serial.println("\nMemulai text-to-speech...");
  return audio.connecttospeech(text.c_str(), "en");
}

String getHFResponse(String query) {
  WiFiClientSecure client;
  client.setInsecure();
  String responseText = "";

  if (!client.connect("huggingface.co", 443)) {
    Serial.println("Gagal terhubung ke server!");
    return "";
  }

  String payload = "{\"model\":\"deepseek-ai/DeepSeek-R1\","
                   "\"messages\":[{\"role\":\"user\",\"content\":\"" + query + "\"}],"
                   "\"max_tokens\":" + String(maxTokens) + ","
                   "\"stream\":true}";

  String request = "POST /api/inference-proxy/together/v1/chat/completions HTTP/1.1\r\n";
  request += "Host: huggingface.co\r\n";
  request += "Authorization: Bearer " + String(hfToken) + "\r\n";
  request += "Content-Type: application/json\r\n";
  request += "Content-Length: " + String(payload.length()) + "\r\n";
  request += "Connection: close\r\n\r\n";
  request += payload;

  client.print(request);

  unsigned long lastDataTime = millis();
  while (client.connected() || client.available()) {
    while (client.available()) {
      String line = client.readStringUntil('\n');
      lastDataTime = millis();

      if (line.startsWith("data: ")) {
        line.remove(0, 6);
        
        if (line == "[DONE]") break;
        
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, line);
        
        if (!error && 
            doc.containsKey("choices") &&
            doc["choices"][0].containsKey("delta") &&
            doc["choices"][0]["delta"].containsKey("content")) {
          String chunk = doc["choices"][0]["delta"]["content"].as<String>();
          responseText += chunk;
        }
      }
      
      if (millis() - lastDataTime > 10000) break;
    }
    if (millis() - lastDataTime > 30000) break;
  }
  
  client.stop();
  return responseText;
}

void printFormatted(String text) {
  int currentLineLength = 0;
  
  for (unsigned int i = 0; i < text.length(); i++) {
    Serial.print(text[i]);
    currentLineLength++;
    
    if (currentLineLength >= SERIAL_WIDTH-1 && text[i] == ' ') {
      Serial.println();
      currentLineLength = 0;
    }
  }
  Serial.println("\n");
}

void cleanTTSText(String &text) {
  // Bersihkan teks untuk TTS
  text.replace("\n", " ");
  text.replace("\r", "");
  text.replace("  ", " ");
  text.replace("\"", "");
  text.replace("\\", "");
  text.replace("...", ".");
  text.trim();
  
  // Pastikan kalimat diakhiri dengan titik
  if(text.charAt(text.length()-1) != '.') {
    text += ".";
  }
}

void audio_info(const char *info) {
  // Tampilkan semua log audio untuk debugging
  Serial.print("[AUDIO] ");
  Serial.println(info);
  
  // Handle error khusus
  if(strstr(info, "error")) {
    Serial.println("Restarting audio...");
    audio.stopSong();
  }
}