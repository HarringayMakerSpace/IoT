/**
 * ESP8266 sketch to monitor soil moisture and temperature, publishing sensor readings once a day. 
 * Connects to the public FON WiFi network so can work out and about the neighbourhood.
 * 
 * Requires:
 *  - a hygrometer probe connected to the ADC pin
 *  - a DS18B20 temperature sensor connected to GPIO-4
 * 
 * Using deepSleep and just one publish per day it should run for over a year on a couple of AA 
 * alkaline batteries. Use an ESP with an external antenna such as an ESP-07 or ESP-201 and bury 
 * it in the ground with just the whip antenna poking out.
 * 
 * The Harringay Maker Space
 * License: Apache License v2
 **/
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h> // https://milesburton.com/Dallas_Temperature_Control_Library

// -------------- Customize these values -----------------
const char* fonUserId = "<bt userid>";
const char* fonPassword = "<bt password>";
const char* fredUserId   = "<FRED USERID>"; 
const char* httpNode = "test1";
// ----------- Customize the above values ----------------

const char* fonssid = "BTWifi-with-FON";
const char* fonHost = "www.btopenzone.com";
const int fonPort = 8443;

const char* host = "fred.sensetecnic.com";

// the httpNode can be shared by lots of sensors so give each a unique id
const char* thisID = "Frobisher-S";

unsigned long oneSecond = 1000 * 1000;   
unsigned long sleepTime = 60 * 60 * oneSecond; // 1 hour  
//unsigned long sleepTime = 1 * oneSecond; 

OneWire oneWire(4); // on GPIO-4
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200); 
  Serial.println(); 

  sleep1Day();
  initWifi();
  logonFON();
  sendReading();
  goToSleep();
}

void loop() {
   // should never get here  
}

void goToSleep() {
  Serial.print("*** Up time: ");
  Serial.print(millis());
  Serial.print(", deep sleeping for ");
  Serial.print(sleepTime);
  Serial.println(" microseconds...");
  ESP.deepSleep(sleepTime, WAKE_RF_DISABLED); // WiFi radio off
}

void sendReading() {
  Serial.print("*** Connecting to ");
  Serial.print(host);
  
  WiFiClient client;
  int retries = 15;
  while (!!!client.connect(host, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if (!!!client.connected()) {
    Serial.println("*** Failed to connect.");
    goToSleep();
  }
  
  // http://fred.sensetecnic.com/public/torntrousers/soil1?vdd=3.0&soil=123 

  int soil = readSoil();
  float temp = readTemp();
            
  String url = "/public/";
  url += fredUserId;
  url += "/";
  url += httpNode;
  url += "?id=";
  url += thisID;
  url += "&soil=";
  url += soil;
  url += "&temp=";
  url += temp;
  url += "&rssi=";
  url += WiFi.RSSI();
  url += "&uptime=";
  url += millis();
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");

  retries = 100; // 5 seconds 
  while(!!!client.available() && (retries-- > 0)) {
    delay(50);             
  }
  // don't need to check for timeout as the next step is sleeping anyway
  
  while(client.available()){
    Serial.write(client.read());
  }
  Serial.println();
}

void initWifi() {
  Serial.print("*** Connecting to ");
  Serial.print(fonssid);

  WiFi.mode(WIFI_STA);   
  WiFi.begin(fonssid);

  int retries = 200; // 20 seconds
  while (WiFi.status() != WL_CONNECTED && (retries-- > 0)) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");

  if (retries < 1) {
    Serial.println("*** Failed to connect to WiFi");
    goToSleep();
  }

  Serial.print("*** WiFi connected, IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print(" RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.print(" BSSID: ");
  Serial.println(WiFi.BSSIDstr());
}

void logonFON() {
  WiFiClientSecure client;
  Serial.print("*** Connecting to ");
  Serial.print(fonHost);
  int retries = 5;
  while (!!!client.connect(fonHost, fonPort) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if (!!!client.connected()) {
    Serial.println("*** Failed to connect to FON");
    goToSleep();
  }

  String url = "/tbbLogon";
  String postData = "username=";
  postData += fonUserId;
  postData += "&password=";
  postData += fonPassword;
  postData += "&xhtmlLogon=https%3A%2F%2Fwww.btopenzone.com%3A8443%2FtbbLogon";
  
  String request = "POST " + url + " HTTP/1.1\r\n"
                "Host: www.btopenzone.com:8443" + "\r\n"
                "User-Agent: curl/7.30.0\r\n"
                "Accept: */*\r\n"
                "Content-Type: application/x-www-form-urlencoded\r\n"
                "Content-Length: " + postData.length() + "\r\n"
                "\r\n" + postData;

  client.print(request);
  Serial.print(request);

  Serial.print("\n Request sent, receiving response");
  retries = 50 * 20 * 6; // 6 seconds
  while (!!!client.available() && (retries-- > 0)) {
    delay(50);
    Serial.print(".");
  }
  Serial.println();

  while (client.available()) {
    Serial.write(client.read());
  }

  Serial.println("*** FON logon done, closing connection");
  client.stop();
}

int readSoil() {
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  delay(100);
  int soil = analogRead(A0);
  digitalWrite(4, LOW);
  Serial.print("Soil reading: ");
  Serial.println(soil);
  return soil;
}

float readTemp() {
  sensors.begin();
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  Serial.print("Temp: ");
  Serial.println(temp);
  return temp;
}

/**
 * The deepSleep time is an uint32 of microseconds so a maximum of 4,294,967,295, about 71 minutes. 
 * So to publish a reading once per day needs to sleep for one hour 24 times and keep track of  
 * where its up to in RTC memory or EEPROM.
 * (RTC memory is broken, see https://github.com/esp8266/Arduino/issues/619) 
 * The WiFi radio is switched off for all but the 24th wakeup to save power
 */
void sleep1Day() {
  byte rtcStore[2];
//  system_rtc_mem_read(64, rtcStore, 2);
  EEPROM.begin(4);
  rtcStore[0] = EEPROM.read(0);
  rtcStore[1] = EEPROM.read(1);
  printRtcStore(rtcStore);

  // 123 in [0] is a marker to detect first use   
  if (rtcStore[0] != 123) {
     rtcStore[0] = 123;
     rtcStore[1] = 0;
  } else {
     rtcStore[1] += 1;
  }
  if (rtcStore[1] > 24) {
     rtcStore[1] = 0;
  }
  printRtcStore(rtcStore);
//  system_rtc_mem_write(64, rtcStore, 2);
  EEPROM.write(0, rtcStore[0]);
  EEPROM.write(1, rtcStore[1]);
  EEPROM.end();
   
  if (rtcStore[1] == 0) {
    return; // a day is up, go do some work
  } 

  Serial.print("*** Up time: ");
  Serial.print(millis());
  Serial.print(", deep sleep for ");
  Serial.print(sleepTime/1000000);
  Serial.print(" seconds, with radio ");
  if (rtcStore[1] == 24) {
     Serial.println("on... ");
     ESP.deepSleep(1, WAKE_RF_DEFAULT);
  } else {
     Serial.println("off... ");
     ESP.deepSleep(sleepTime, WAKE_RF_DISABLED);
  }
}

void printRtcStore(byte* rtcStore) {
  Serial.print("rtcStore: ");
  Serial.print(rtcStore[0]);
  Serial.print(",");
  Serial.print(rtcStore[1]);
  Serial.println();
}

