#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "HURAA";
const char* password = "12345678";
const char* HF_Token = "";
const char* Max_Tokens = "500";
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
    delay(100);
  }

  res = Serial.readStringUntil('\n');
  res.trim();
  
  if (res.length() == 0) {
    Serial.println("Input kosong! Silakan coba lagi.");
    return;
  }

  Serial.print("\nYou: ");
  Serial.println(res);

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(5000);  // Set timeout 5 detik

  if (!client.connect("huggingface.co", 443)) {
    Serial.println("Gagal terhubung ke server!");
    return;
  }

  String payload = "{\"model\": \"deepseek-ai/DeepSeek-R1\", \"messages\": [{\"role\": \"user\", \"content\": \"" + res + "\"}], \"max_tokens\": " + String(Max_Tokens) + ", \"stream\": true}";

  String request = "POST /api/inference-proxy/together/v1/chat/completions HTTP/1.1\r\n";
  request += "Host: huggingface.co\r\n";
  request += "Authorization: Bearer " + String(HF_Token) + "\r\n";
  request += "Content-Type: application/json\r\n";
  request += "Content-Length: " + String(payload.length()) + "\r\n";
  request += "Connection: close\r\n\r\n";
  request += payload;

  client.print(request);

  Serial.println("\nAssistant: ");
  
  unsigned long lastDataTime = millis();
  String buffer = "";
  
  while (client.connected() || client.available()) {
    while (client.available()) {
      String line = client.readStringUntil('\n');
      lastDataTime = millis();  // Reset timer
      
      if (line.startsWith("data: ")) {
        line.remove(0, 6);
        
        if (line == "[DONE]") break;
        
        // Filter karakter non-printable
        line.replace("[^\x20-\x7E]", "");
        
        // Cek validitas JSON dasar
        if (line.length() > 10 && line.indexOf("{") >= 0 && line.indexOf("}") >= 0) {
          DynamicJsonDocument doc(2048); // Meningkatkan buffer JSON
          DeserializationError error = deserializeJson(doc, line);
          
          if (!error) {
            // Validasi struktur JSON
            if (doc.containsKey("choices") && 
                doc["choices"][0].containsKey("delta") && 
                doc["choices"][0]["delta"].containsKey("content")) {
                  
              String chunk = doc["choices"][0]["delta"]["content"].as<String>();
              if (chunk.length() > 0) {
                Serial.print(chunk);
                buffer = ""; // Reset buffer
              }
            }
          } else {
            Serial.print("?");
            buffer += line; // Simpan data yang gagal diparse
            if (buffer.length() > 512) buffer = ""; // Prevent buffer overflow
          }
        }
      }
      
      // Timeout handling
      if (millis() - lastDataTime > 10000) {
        Serial.println("\n\nTimeout: Tidak ada data baru!");
        break;
      }
    }
    
    // Cek timeout keseluruhan
    if (millis() - lastDataTime > 30000) {
      Serial.println("\n\nKoneksi timeout!");
      break;
    }
  }
  
  Serial.println("\n\n[END]");
  client.stop();
  res = "";
}