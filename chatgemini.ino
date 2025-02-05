#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "you ssid";
const char* password = "you-password";
const char* Gemini_Token = "api key";
const char* Gemini_Max_Tokens = "100";
String res = "";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  Serial.println("\nAsk something: ");
  
  while (!Serial.available()) {
    delay(100); // Hindari looping berlebihan
  }

  res = Serial.readStringUntil('\n'); // Baca input pengguna hingga enter ditekan
  res.trim(); // Hapus spasi di awal dan akhir
  
  if (res.length() == 0) {
    Serial.println("Input kosong! Silakan coba lagi.");
    return;
  }

  Serial.print("\nYou: ");
  Serial.println(res);

  HTTPClient https;
  WiFiClientSecure client;
  client.setInsecure(); // Gunakan koneksi tanpa verifikasi SSL

  String apiUrl = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + String(Gemini_Token);
  
  if (https.begin(client, apiUrl)) {
    https.addHeader("Content-Type", "application/json");

    String payload = "{\"contents\": [{\"parts\":[{\"text\":\"" + res + "\"}]}], \"generationConfig\": {\"maxOutputTokens\": " + String(Gemini_Max_Tokens) + "}}";

    int httpCode = https.POST(payload);

    if (httpCode > 0) {
      String response = https.getString();
      
      DynamicJsonDocument doc(4096); // Buffer JSON lebih besar untuk respons panjang
      DeserializationError error = deserializeJson(doc, response);

      if (!error) {
        String answer = doc["candidates"][0]["content"]["parts"][0]["text"].as<String>();

        answer.trim();
        
        Serial.println("\nGemini: ");
        Serial.println(answer);
      } else {
        Serial.println("Gagal parsing JSON!");
      }
    } else {
      Serial.printf("Error HTTP: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
  } else {
    Serial.println("Gagal terhubung ke API Gemini!");
  }

  res = ""; // Reset input
}
