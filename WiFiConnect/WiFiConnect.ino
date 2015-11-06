/*
 *  Sample code for a function that takes a list of WiFi SSIDs and connects to the one with the strongest signal
 *  
 *  The Harringay Maker Space
 *  Apache License v2
 */
#include "ESP8266WiFi.h"

// WiFi connect timeout (seconds)
const int TIMEOUT = 3; 

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  delay(5000);
  Serial.println("Setup done");
}

void loop() {

  wifiBestConnect("BTWifi-with-FON,someNet+somePswd,BTHub5-72W5+xxxxxxxxxx,anotherOpenNet,andAnotherNet+pswd");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected to: "); Serial.print(WiFi.SSID()); 
    Serial.print(", BSSID: "); Serial.print(WiFi.BSSIDstr());
    Serial.print(", Channel: "); Serial.print(WiFi.channel());
    Serial.print(", RSSI: "); Serial.print(WiFi.RSSI());
    Serial.print(", IP: "); Serial.print(WiFi.localIP());
    Serial.println();
  } else {
    Serial.println("*** Failed to connect to any WiFi network");
  }
  
  delay(5000);
  WiFi.disconnect();
}

/**
 * Connected to the SSID with the strongest signal.
 * The ssid paramter is a comma seperated list of SSID's. If an SSID has a 
 * password then append the password to the SSID with a plus '+' character.
 * Eg. wifiBestConnect("myNet+myPswd,someOpenNetwork,someOtherNetwork+password")
 */
void wifiBestConnect(String ssids) {
  Serial.print("Scanning...");
  int n = WiFi.scanNetworks();
  Serial.print(n); Serial.println(" networks found");
  byte tried[n];
  memset(tried,0,sizeof(tried));
  while (tryConnect(n, ssids, tried)) {
  }
}

/* Connect to the SSID with the srongest RSSI.
 *  n is the number of networks available from the scan
 *  ssids is a list of desired SSIDs and their passwords
 *  tried is an array of flags for networks to ignore as they've already been tried
 */
boolean tryConnect(int n, String ssids, byte tried[]) {
  if (n == 0) return false;
  int bestSSID = -1;
  boolean moreToTry = false;
  for (int i = 0; i < n; ++i) {
    String s = String(WiFi.SSID(i));
    if (!!!tried[i] && ssids.indexOf(s) > -1) {
      if (bestSSID < 0) {
        bestSSID = i;
      } else if (WiFi.RSSI(i) > WiFi.RSSI(bestSSID)) {
        bestSSID = i; 
        moreToTry = true;
      }
    }
  }

  if (bestSSID > -1) {
    tried[bestSSID] = 1;
    String pswd;
    if (WiFi.encryptionType(bestSSID) != ENC_TYPE_NONE) {
      String ssid = String(WiFi.SSID(bestSSID));
      int ps = ssids.indexOf(ssid) + ssid.length() + 1;
      int pe = ssids.indexOf(',', ps);
      if (pe < 0) {
        pswd = ssids.substring(ps); 
      } else {
        pswd = ssids.substring(ps, pe); 
      }
    }
    Serial.print("Connecting to: "); Serial.print(WiFi.SSID(bestSSID)); Serial.print(" "); Serial.print(WiFi.BSSIDstr(bestSSID));
    WiFi.begin(WiFi.SSID(bestSSID), pswd.c_str(), WiFi.channel(bestSSID), WiFi.BSSID(bestSSID));  
    int timeout = TIMEOUT * 10;
    while (WiFi.status() != WL_CONNECTED && (timeout-- > 0)) {
      delay(100);
      Serial.print(".");
    }
    Serial.println();
  }
  return WiFi.status() != WL_CONNECTED && moreToTry;
}

