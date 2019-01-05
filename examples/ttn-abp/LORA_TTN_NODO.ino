/***
 * Original Code: 
 * Modified By: Bikash Narayan Panda (weargeniuslabs@gmail.com) 
 * ***/
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <U8x8lib.h>


static const PROGMEM u1_t NWKSKEY[16] ={ 0xBA, 0x4C, 0x2D, 0xF8, 0x99, 0x1E, 0x25, 0x35, 0xEB, 0x41, 0x41, 0x63, 0x73, 0xCC, 0x28, 0xEE };  //MSB
//static const PROGMEM u1_t NWKSKEY[16] ={ 0xEE, 0x28, 0xCC, 0x73, 0x63, 0x41, 0x41, 0xEB, 0x35, 0x25, 0x1E, 0x99, 0xF8, 0x2D, 0x4C, 0xBA };  //LSB
static const PROGMEM u1_t APPSKEY[16] ={ 0x0B, 0x8E, 0xB4, 0x95, 0xE7, 0xF5, 0xBD, 0x43, 0x1C, 0x81, 0x55, 0xE6, 0xFA, 0x32, 0x64, 0x04 }; //MSB
//static const PROGMEM u1_t APPSKEY[16] ={ 0x04, 0x64, 0x32, 0xFA, 0xE6, 0x55, 0x81, 0x1C, 0x43, 0xBD, 0xF5, 0xE7, 0x95, 0xB4, 0x9E, 0x0B };  //LSB
static const u4_t DEVADDR = 0x26011678; // <-- Change this address for every node! //MSB
//static const u4_t DEVADDR = 0x78160126; // <-- Change this address for every node! //LSB

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t mydata[] = "por los Machetes";
static osjob_t sendjob;


// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 14,
  //.dio = {26, 33, 32}
  .dio = {26, 34, 35}
};
//OLED Declaration 
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

//LED and Button Pins
int buttonPin = 13;
int ledPin=12;
int boardLED=25;
int lastState=0;

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_TXCOMPLETE:
           ledFLash(2);
           Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
           u8x8.drawString(0, 2, "Data Sent");
           u8x8.drawString(0, 4, "Button Released");
           if(LMIC.dataLen) {
               // data received in rx slot after tx
               Serial.print(F("Data Received: "));
               u8x8.drawString(0, 3, "Data Received: ");
               Serial.print(LMIC.dataLen);
               Serial.print(F(" bytes for downlink: 0x"));
               for (int i = 0; i < LMIC.dataLen; i++) {
                   if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
                       Serial.print(F("0"));
                   }
                   Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
               }
               Serial.println();
           }
           // Schedule next transmission
           os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(5), do_send);
           delay(2000);
           u8x8.clearLine(1);
           u8x8.clearLine(2);
           u8x8.clearLine(3);
           u8x8.clearLine(4);
            break;
         default:
            Serial.println(F("Unknown event"));
            u8x8.drawString(0, 2, "Unknown event");
            break;
    }
}

void ledFLash(int flashes){
    int lastStateLED=digitalRead(ledPin);
    for(int i=0;i<flashes;i++){
        digitalWrite(ledPin, HIGH);
        delay(300);
        digitalWrite(ledPin, LOW);
        delay(300);
    }
    digitalWrite(ledPin,lastStateLED);
}
void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    u8x8.drawString(0, 4, "Button Pressed");
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
        u8x8.drawString(0, 1, "OP_TXRXPEND, not sending");
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
        u8x8.drawString(0, 1, "Packet queued");
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    //setup the display
    u8x8.begin();
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.drawString(0, 0, "WGLabz LoRa Test");

    Serial.begin(115200);
    Serial.println(F("Starting"));

    //In/Out Pins
    pinMode(ledPin, OUTPUT);
    pinMode(buttonPin, INPUT);
    digitalWrite(buttonPin, HIGH);

    // LMIC init &RESET
    os_init();
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

   LMIC_setupChannel(0, 903900000, DR_RANGE_MAP(DR_SF10, DR_SF7),  BAND_CENTI);      // g-band
   LMIC_setupChannel(1, 904100000, DR_RANGE_MAP(DR_SF10, DR_SF7B), BAND_CENTI);      // g-band
   LMIC_setupChannel(2, 904300000, DR_RANGE_MAP(DR_SF10, DR_SF7),  BAND_CENTI);      // g-band
   LMIC_setupChannel(3, 904500000, DR_RANGE_MAP(DR_SF10, DR_SF7),  BAND_CENTI);      // g-band
   LMIC_setupChannel(4, 904700000, DR_RANGE_MAP(DR_SF10, DR_SF7),  BAND_CENTI);      // g-band
   LMIC_setupChannel(5, 904900000, DR_RANGE_MAP(DR_SF10, DR_SF7),  BAND_CENTI);      // g-band
   LMIC_setupChannel(6, 905100000, DR_RANGE_MAP(DR_SF10, DR_SF7),  BAND_CENTI);      // g-band
   LMIC_setupChannel(7, 905300000, DR_RANGE_MAP(DR_SF10, DR_SF7),  BAND_CENTI);      // g-band
   LMIC_setupChannel(8, 904600000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

   // LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
   // LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
   // LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
   // LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
   // LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
   // LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
   // LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
   // LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
   // LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

    // Disable link check validation
    LMIC_setLinkCheckMode(0);
    LMIC.dn2Dr = DR_SF9;
    // Set data rate and transmit power (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF7,14);

    // In my place we are not using 868 MHz
    LMIC_disableChannel(0);
    LMIC_disableChannel(1);
    LMIC_disableChannel(2);
}

void loop() {
    os_runloop_once();
    if(!digitalRead(buttonPin)){
        //
        delay(400);
        do_send(&sendjob);
        //
    }
}

//BA4C2DF8991E2535EB41416373CC28EE
//BA4C2DF8991E2535EB41416373CC28EE

//0B8EB495E7F5BD431C8155E6FA326404
//0B8EB495E7F5BD431C8155E6FA326404


//application/144279516353570/device/0089F87F43B468D1/r
