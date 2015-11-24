/*
 * This sketch shows an example of sending a reading to data.sparkfun.com once per day.
 * 
 * It uses the Sparkfun testing stream so the only customizing required is the WiFi SSID and password.
 * 
 * The Harringay Maker Space
 * License: Apache License v2
 * 
 */
#include <ESP8266WiFi.h>
#include <ESPDailyTask.h>

const char* ssid     = "your-ssid";
const char* password = "your-password";

// Set this for what time your daily code should run 
ESPDailyTask dailyTask(11*60 + 15); // 11:15am

// Use Sparkfun testing stream
const char* host = "data.sparkfun.com";
const char* streamId   = "Jxyjr7DmxwTD5dG1D1Kv";
const char* privateKey = "gzgnB4VazkIg7GN1g1qA";

void setup() {
  Serial.begin(115200); Serial.println();

  dailyTask.sleep1Day();

  // put your daily code here...
  initWifi();
  sendReadings();

  // and back to sleep once daily code is done  
  dailyTask.backToSleep();
}

void loop() {
  // sleeping so wont get here
}

void sendReadings() {
  Serial.print("Connecting to "); Serial.print(host);
  
  WiFiClient client;
  int retries = 5;
  while (!!!client.connect(host, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if (!!!client.connected()) {
     Serial.println("Failed to connect, going back to sleep");
     dailyTask.backToSleep();
  }

  String url = "/input/";
  url += streamId;
  url += "?private_key=";
  url += privateKey;
  url += "&brewTemp=";
  url += WiFi.macAddress(); // Could be any string
  
  Serial.print("Request URL: "); Serial.println(url);
  
  client.print(String("GET ") + url + 
                  " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" + 
                  "Connection: close\r\n\r\n");

  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  
  if (!!!client.available()) {
     Serial.println("No response, going back to sleep");
     dailyTask.backToSleep();
  }

  dailyTask.timeAdjustFromDateHeader(client);    

  while(client.available()){
    Serial.write(client.read());
  }
  
  Serial.println("\nclosing connection");
  client.stop();
}

void initWifi() {
  Serial.print("Connecting to: "); Serial.print(ssid);

  if (strcmp (WiFi.SSID(),ssid) != 0) {
    WiFi.begin(ssid, password);  
  }

  int timeout = 10 * 4; // 10 seconds
  while (WiFi.status() != WL_CONNECTED  && (timeout-- > 0)) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");

  if (WiFi.status() != WL_CONNECTED) {
     Serial.println("Failed to connect, going back to sleep");
     dailyTask.backToSleep();
  }

  Serial.print("WiFi connected in: "); Serial.print(millis());
  Serial.print(", IP address: "); Serial.println(WiFi.localIP());
}
