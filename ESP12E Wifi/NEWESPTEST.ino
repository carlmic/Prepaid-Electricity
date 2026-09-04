// MODEM MODE: Transparent Bridge
#include <ESP8266WiFi.h>

char ssid[] = "Kuroshi"; 
char pass[] = "kasiiloveyou"; // ur wifi password!
const char* server = "blynk.cloud";
const int port = 80;

WiFiClient client;

void setup() {
  Serial.begin(9600); // 9600 baud for Arduino compatibility
  
  WiFi.disconnect();
  // WiFi.setOutputPower(5); // Keep this commented out if it helped connection!
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {
  if (!client.connected()) {
    if (!client.connect(server, port)) {
      delay(500);
      return;
    }
  }
  if (Serial.available()) {
    client.write(Serial.read());
  }
  if (client.available()) {
    Serial.write(client.read());
  }
}