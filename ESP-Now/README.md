*UPDATE:* some more recent ESP-Now code is [here](https://github.com/HarringayMakerSpace/ESP-Now).

# ESP-Now testing

 Trying out ESP-Now 
 See https://espressif.com/en/products/software/esp-now/overview

 ESP-Now enables fast lightwieght connectionless communication between ESP8266's.

 It sounds like you could have a battery powered remote sensor node sending readings with ESP-Now.
 From trying this an ESP8266 using deepSleep can wake up, do the ESP-Now send, and 
 go back to deepSleep in about 270 milliseconds, so should get great battery life
 comparable to using BLE but without all the BLE compexity.

 However, presently I haven't got the ESP-Now receiver node (slave) to work reliably while 
 also connected to another WiFi Access Point. See http://bbs.espressif.com/viewtopic.php?f=7&t=2514&p=8285
  
