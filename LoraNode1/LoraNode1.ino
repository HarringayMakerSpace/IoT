/* ESP8266 with RFM95W/SX1276 Example
 *  
 * Uses deepSleep and publishes Vcc every 30 seconds.
 *
 * Tested using:
 *  - ESP8266/Arduino 2.2.0
 *  - Arduino LMIC 1.5.0+arduino-1
 *  - Arduino IDE 1.6.8 
 *
 * Wiring connections:
 * ESP8266  - SX1276
 *  GND        GND
 *  VCC        VCC
 *  GPIO15     NSS
 *  GPIO12     MOSI
 *  GPIO13     MIS0
 *  GPIO14     SCK
 *  GPIO5      DIO0
 *  GPIO4      DIO1
 *
 * Also on the ESP connect GPIO16-RST for deepSleep wakeup
 *  
 * Spare pins for other things:
 *  GPIO0 - must be high to boot
 *  GPIO1 - Serial TX, will output some garbage at bootup 
 *  GPIO2 - must be high to boot
 *  GPIO3 - Serial RX
 *  ADC
 *  
 * Author: Ant Elder
 * License: Apache License v2
 */

#include <ESP8266WiFi.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

ADC_MODE(ADC_VCC); // enables reading the ESP8266 supply voltage

#define SLEEP_SECONDS 30

const lmic_pinmap lmic_pins = {
    .nss = 15,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,
    .dio = {5, 4, LMIC_UNUSED_PIN},
};

// LoRaWAN end-device address (DevAddr) see http://thethingsnetwork.org/wiki/AddressSpace
static const u4_t DEVADDR = 0x03FF0701 ; // <-- Change this address for every node!

volatile boolean dio0Triggered;

void setup() {
  Serial.begin(115200); Serial.println();

  lmicInit();
  // LMIC bug? the EV_TXCOMPLETE event seems to takes ages to happen, but DIO0 high indicates TX complete 
  attachInterrupt(digitalPinToInterrupt(lmic_pins.dio[0]), dio0ISR, RISING);

  String vcc = String(ESP.getVcc());   
  do_send(vcc);
}

// loop runs until the TX Complete event puts the ESP to sleep
void loop() {
  os_runloop_once();
  if (dio0Triggered) goToSleep();
}

void dio0ISR() {
   dio0Triggered = true;  
}

void do_send(String msg) {
   Serial.print("Sending msg at "); Serial.print(millis());
   Serial.print(": "); Serial.print(msg); Serial.println();

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
      Serial.println("OP_TXRXPEND, not sending");
      return;
    }

    Serial.print("Sending on channel: "); Serial.println(LMIC.txChnl);

    dio0Triggered = false;
    LMIC_setTxData2(1, (xref2u1_t)msg.c_str(), msg.length(), 0);
}

void onEvent (ev_t ev) {
    Serial.print("on event: "); Serial.println(ev);
    switch(ev) {
      case EV_TXCOMPLETE:
          Serial.print("Event EV_TXCOMPLETE at "); Serial.println(millis());
          if(LMIC.dataLen) { // has data been received in rx slot after tx
                Serial.print(F("Data Received: "));
                Serial.write(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
                Serial.println();
           }
          goToSleep();          
          break;
       default:
          break;
    }
}

void goToSleep() {
  Serial.print("Up time "); Serial.print(millis());
  Serial.print(", going to sleep for "); Serial.print(SLEEP_SECONDS); Serial.println(" secs");
  ESP.deepSleep(SLEEP_SECONDS*1000000, RF_DISABLED);  
}

// LoRaWAN NwkSKey, network session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const PROGMEM u1_t NWKSKEY[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

// LoRaWAN AppSKey, application session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const u1_t PROGMEM APPSKEY[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

void lmicInit() {
  Serial.print("LMIC Init started at "); Serial.print(millis()); Serial.print("...");
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly 
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    // Set up the channels used by the Things Network, which corresponds
    // to the defaults of most gateways. Without this, only three base
    // channels from the LoRaWAN specification are used, which certainly
    // works, so it is good for debugging, but can overload those
    // frequencies, so be sure to configure the full frequency range of
    // your network here (unless your network autoconfigures them).
    // Setting up channels should happen after LMIC_setSession, as that
    // configures the minimal channel set.
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // Set data rate and transmit power (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF7,14);

  // only use channel 0 for the single channel gateway
  LMIC_disableChannel(1);
  LMIC_disableChannel(2);
  LMIC_disableChannel(3);
  LMIC_disableChannel(4);
  LMIC_disableChannel(5);
  LMIC_disableChannel(6);
  LMIC_disableChannel(7);
  LMIC_disableChannel(8);

  Serial.print("done "); Serial.println(millis());
}

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }
