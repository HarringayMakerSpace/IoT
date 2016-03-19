/*
Publishes gas usage from a domestic gas meter to the Watson IoT Platform.

Most domestic gas meters are "pulse ready" - there is magnet on the lowest meter number so you 
can use a magnetic sensor to count the revolutions of the dial and so measure the gas consumption. 
This uses a simple reed switch connected to a GPIO pin on an ESP8266 to sense the magnet. The 
exact placement of the reed switch doesn't seem to fussy, just held on to the underneath the 
smallest dial with a bit of bluetack and duct tape.

My gas bill describes how to get the cost:
---
Your gas meter measures usage in units, but like all suppliers, we have to do a bit of 
maths to turn it into kWh. Here's how it works:
GAS UNITS USED X CALORIFIC VALUE (39.4) X VOLUME CORRECTION (1.02264) / 3.6
---
My tarrif costs 2.838p per kWH. One revolution of the smallest dial is equal to 10 cubic decimetres
of gas which is one thousandth of a unit, so costs: 0.001 x 39.4 * 1.02264 / 3.6 = 0.03176 pence

Ant Elder
License: Apache License v2
*/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

//-------- Customise these values -----------
const char* ssid = "<yourWifiSSID>";
const char* password = "<youWifiPswd>";

#define ORG "<yourOrg>" // Watson IoT Platform organization
#define DEVICE_TYPE "ESP8266" // Watson registered device type
#define DEVICE_ID "GasMeter" // Watson registered device id
#define TOKEN "<yourDeviceToken>" // your Watson device token
#define EVENT "gasReading" 
//-------- Customise the above values --------
                                   
#define REED_SWITCH_GPIO 12

const int PUBLISH_INTERVAL = 60 * 60 * 1000; // One hour in milliseconds

// updated in the interrupt function so must be volatile to ensure updates are saved
volatile unsigned int gasUsage = 0;
volatile unsigned long lastUpdate = 0;

String watsonUrl = "https://use-token-auth:" TOKEN "@" ORG ".internetofthings.ibmcloud.com/api/v0002/device/types/" DEVICE_TYPE "/devices/" DEVICE_ID "/events/" EVENT;

unsigned long nextPublishMillis = 0;
unsigned long debugOps = 0;

void setup() {
   Serial.begin(115200); Serial.println();

   pinMode(REED_SWITCH_GPIO, INPUT_PULLUP);
   // use an interupt function to count the meter pulses so we don't miss pulses
   attachInterrupt(digitalPinToInterrupt(REED_SWITCH_GPIO), gasUseISR, FALLING);

   Serial.print("Connecting WiFi to "); Serial.print(ssid); Serial.print("...");
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);
   WiFi.waitForConnectResult();
   Serial.print("connected, IP address: "); Serial.println(WiFi.localIP());

   setNextPublishMillis();
}

void loop() {
  if (millis() > nextPublishMillis) {
     publishGasUse();
     setNextPublishMillis();
  }

  // debug output every 10 secs just to see its running
  if ((millis() - debugOps) > 10000) { 
    Serial.print("gasUsage = "); Serial.print(gasUsage);
    Serial.print(", next publish in "); Serial.print((nextPublishMillis - millis())/1000); Serial.println(" seconds");
    debugOps = millis();
  }
}

void publishGasUse() {
   HTTPClient http;
   http.begin(watsonUrl);
   http.addHeader("Content-Type", "application/json");
   String payload = String("{ \"d\": {\"consumption\": ") + getAndResetGasUse() + "} }"; 
   Serial.print("Publish payload: "); Serial.println(payload);
   int httpCode = http.POST(payload);
   Serial.print("HTTP POST Response: "); Serial.println(httpCode); // HTTP code 200 means ok 
   http.end();
}

// The ESP clock drifts and runs a bit slow so to prevent the reading publishes drifting too
// this gets the current time from the Internet to have the publishes on the hour every hour 
void setNextPublishMillis() {
   HTTPClient http;
   http.begin("http://google.com/");

   const char * headerkeys[] = {"Date"};
   size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
   http.collectHeaders(headerkeys,headerkeyssize);
   
   http.sendRequest("HEAD");
   String dateHeader = http.header("Date");   
   int i = dateHeader.lastIndexOf(':');
   int milliSecsPastHour = 0;      
   if (i != -1) {
      int hours = dateHeader.substring(i-5, i-3).toInt();
      int minutes = dateHeader.substring(i-2, i).toInt();
      int seconds = dateHeader.substring(i+1, i+3).toInt();
      Serial.print("Current time ");Serial.print(hours);Serial.print(":");Serial.print(minutes);Serial.print(":");Serial.println(seconds);
      milliSecsPastHour = ((minutes * 60) + seconds) * 1000;
   }

   nextPublishMillis = millis() + (PUBLISH_INTERVAL - milliSecsPastHour);
   Serial.print("nextPublishMillis set to: "); Serial.println(nextPublishMillis);
}

int getAndResetGasUse() {
   int value = gasUsage;
   gasUsage = 0;
   // its possible the ISR may be triggered and do a concurrent update of the gasUsage
   // variable while this reset function is running and so corrupt the gasUsage variable, 
   // but we know the ISR will only update once every 4 seconds so just reseting to one
   // again here will fix this (reset to one not zero so as to count the ISR trigger)
   if (gasUsage > 1) gasUsage = 1;   

   return value;
}

//Interupt function that triggers whenever the gas use reed switch is triggered
void gasUseISR() {

  // the reed switch may trigger multiple times as the magnet approaches and moves away
  // this "debounces" that by ignoring triggers within 4 seconds of the previous update. 
  // (With all my gas appliances turned on full it takes a good 9 seconds or so for a 
  // revolution so 4 seconds seems a safe debounce value) 
  if ((millis() - lastUpdate) < 4000) return;

  gasUsage += 1;
  lastUpdate = millis();
}

