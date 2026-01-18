/*
  ESP32 internet Radio
  Yves BOURDON (c) 2023
*/

// ESP32dev Signal   Wired to TFT       Wired to PCM5102      Wired to the rest
// -------- ------   ----------------   -------------------   --------------------------------
// GPIO32            -                  -                     -
// GPIO04            -                  -                     Mute switch      
// GPIO27            pin 5 D/C or A0    -                     -       
// GPIO22            -                  -                     -    
// GPIO16   RXD2     -                  -                     -       
// GPIO17   TXD2     -                  -                     -       
// GPIO18   SCK      pin 7 CLK or SCK   -                     -    
// GPIO19   MISO     -                  -                     -    
// GPIO23   MOSI     pin 6 DIN or SDA   -                     -    
// GPIO14            pin 3 CS           -                     -       
// GPI003   RXD0     -                  -                     -
// GPIO01   TXD0     -                  -                     -
// GPIO35   -        -                  -                     Infrared receiver VS1838B
// GPIO22   -        -                  -                     Volume - switch
// GPIO21   -        -                  -                     Volume + switch
// GPIO32   -        -                  -                     Chanel - switch
// GPIO25   -        -                  -                     Chanel + switch
// GPIO26   -        -                  pin 4 LCK             - 
// GPIO05   -        -                  pin 2 BLCK            -
// GPIO33   -        -                  pin 3 DIN             -
// GPIO02   -        -                  -                     red LED (IR received)
// GPIO17   -        -                  -                     Reserved PSRAM Clock 
// GPIO16   -        -                  -                     Reserved PSRAM CS 
// -------  ------   -----------------  -------------------   -------------------------------   
// GND      -        pin 2 GND          pin 5 GND             Power supply GND
// VCC 3.3V -        pin 8 BL           -                     Power supply
// VCC 3.3V -        pin 1 VCC          pin 6 VIN             Power supply
// EN       -        pin 4 RST          -                     Reset ESP32

// Update ELEGANTOTA_USE_ASYNC_WEBSERVER to 1 in ElegantOTA library
// #ifndef ELEGANTOTA_USE_ASYNC_WEBSERVER
//   #define ELEGANTOTA_USE_ASYNC_WEBSERVER 0
// #endif

#include "Arduino.h"
#include <Adafruit_GFX.h>               // core graphics library
#include <Adafruit_ST7735.h>            // hardware-specific library for ST7735
#include <SPI.h>
#include "Audio.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include <ElegantOTA.h>

#define I2S_DOUT               33
#define I2S_BCLK               05
#define I2S_LRC                26
Audio audio;

#define LED_BUILTIN            02       // red led to show IR activity
#define BLUE_LED               00       // blue led 
#define IR_RECEIVE_PIN         35  

// ---------- MAX7219 connection ----------------------------------------------
#define TFT_CS                 14
#define TFT_RST                -1       // connected to EN (reset) of ESP32
#define TFT_DC                 27

// ---------- Button connection ----------------------------------------------
#define CH_UP                  25       // Station +                          
#define CH_DOWN                32       // Station -
#define VOLUME_UP              21       // Volume +                          
#define VOLUME_DOWN            22       // Volume -
#define MUTE                   04       // Mute

// ---------- include IR library ----------------------------------------------
#define DECODE_RC6                            
#include <IRremote.hpp>                      // https://github.com/Arduino-IRremote/Arduino-IRremote   v 4.0.0

String ssid         = "Bureau";              // "yourSSID";
String password     = "enablemeplease";      // "yourPassword";
String rssi;

String NEW_NAME     = "12 char max";
String NEW_URL      = "paste/copy url after http://   like:   europe2.lmn.fm/europe2.aac";
String EMPTY        = "empty";

// http://fluxradios.blogspot.com/ 
// indicate stream, displayed name for each channel (MAX 12 characters !!!) - Default values stored in Preference 2
String stations[] ={
   "audio.bfmtv.com/rmcradio_128.mp3",                                   "RMC",
   "audio.bfmtv.com/bfmbusiness_128.mp3",                                "BFM Business",
   "cdn.nrjaudio.fm/audio1/fr/30201/aac_64.mp3?origine=fluxradios",      "Cherie FM", 
   "europe2.lmn.fm/europe2.aac",                                         "Europe 2",
   "media.autorouteinfo.fr:8000/direct_sud.mp3",                         "Autor. Info",
   "icecast.radiofrance.fr/fip-hifi.aac",                                "FIP",
   "direct.francebleu.fr/live/fbprovence-midfi.mp3",                     "Fr Bleu Prov",
   "icecast.radiofrance.fr/franceculture-hifi.aac",                      "Fr Culture",
   "direct.franceinfo.fr/live/franceinfo-hifi.aac",                      "France Info",
   "icecast.radiofrance.fr/franceinter-hifi.aac",                        "Fr Inter",
   "scdn.nrjaudio.fm/adwz2/fr/30401/mp3_128.mp3?origine=fluxradios",     "Rires&Chanso",
   "streaming.radio.rtl.fr:80/rtl-1-44-128",                             "RTL",
   "direct.franceinter.fr/live/francemusique-hifi.aac",                  "Fr Musique",
   "icecast.funradio.fr/fun-1-44-128?listen=webCwsBCggNCQgLDQUGBAcGBg",  "Fun Radio",
   "icecast.skyrock.net/s/natio_aac_128k",                               "Skyrock",
   "kissfm2.ice.infomaniak.ch/kissfm2-128.mp3",                          "Kiss FM",
   "jazzradio.ice.infomaniak.ch/jazzradio-high.mp3",                     "Jazz Radio",
   "cdn.nrjaudio.fm/audio1/fr/30601/mp3_128.mp3?origine=fluxradios",     "Nostalgie",
   "cdn.nrjaudio.fm/audio1/fr/30001/aac_64.mp3",                         "NRJ",
   "radioclassique.ice.infomaniak.ch/radioclassique-high.mp3",           "Radio Class",
   "ais-live.cloud-services.paris:8000/rfm.aac",                         "RFM",
   "icecast.rtl2.fr/rtl2-1-44-128?listen=webCwsBCggNCQgLDQUGBAcGBg",     "RTL2",
   "stream.europe1.fr/europe1.aac",                                      "Europe 1"
};

const String Version           = "v1.71 11-03-2023";   

const int   CMD_NEXT_STATION   = 0x20;                             // IR code to be adapted to your remote
const int   CMD_PREV_STATION   = 0x21;
const int   CMD_VOLUME_UP      = 0x10;
const int   CMD_VOLUME_DOWN    = 0x11;
const int   CMD_RESTART        = 0x0c;
const int   CMD_MUTE           = 0x0d;
const int   CMD_PAUSE          = 0x2c;

const int   MAX_STATION        = 50;                               // to be adjusted regarding the number of stored stations and avalable space in memory
const int   MIN_STATION        = 1;

const int   MIN_VOLUME         = 1;                                // 0 is mute
const int   MAX_VOLUME         = 21;

const int   DELAY_CHANEL       = 200;                              // debouncing and repeat variables
bool        flag_chanUp        = 0;                                // change chanel every 200mS if key continues to be pressed
long        t0_chanDown;
bool        flag_chanDown      = 0;
long        t0_chanUp;
const int   DELAY_VOLUME       = 150;                              // change volume every 150mS if key continues to be pressed                   
bool        flag_volDown       = 0;
long        t0_volDown;
bool        flag_volUp         = 0;
long        t0_volUp;
const int   DELAY_MUTE         = 500;                              // switch from muto on to mute off if key continues to be pressed
bool        flag_volMute       = 0;
long        t0_volMute;
const int   DELAY_VOLUME_IR    = 70;                               // change volume every 70mS if key continues to be pressed   
bool        flag_volIrDown     = 0;
long        t0_volIrDown;
bool        flag_volIrUp       = 0;
long        t0_volIrUp;
const int   DELAY_MUTE_IR      = 500;                              // switch from muto on to mute off if key continues to be pressed
bool        flag_volIrMute     = 0;
long        t0_volIrMute;
const int   DELAY_CHANEL_IR    = 300;                              // change chanel every 500mS if key continues to be pressed
bool        flag_chanIrDown    = 0;
long        t0_chanIrDown;
bool        flag_chanIrUp      = 0;
long        t0_chanIrUp;

const int   DELAY_RSSI         = 2000;                             // updates rssi every 2s
long        t0_rssi;

byte        maxStation;                                            // defined at Setup
byte        cur_station;                                           // default station
byte        cur_volume;
bool        flag_mute          = 0;
String      Name;
String      Url;

String      AudioInfo;
String      Station;
String      Title;

char        NTPtime[17]        = "jj/mm/aaaa hh:mm";               // heure NTP
byte        Heure, Minute, Minute_old,Seconde, Jour, Mois;
int         Annee;
const char* ntpServer          = "pool.ntp.org";
const long  gmtOffset_sec      = 3600;
const int   daylightOffset_sec = 0;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
uint8_t   screenWidth          = 160;
uint8_t   screenHeight         = 128;

char APName[]                  = "ESP32-RADIO";                    // Pour connexion à 192.68.4.1 (no Password)
char APIp[]                    = "192.168.4.1";                    // Valeur de connexion par défaut
bool modeAP                    = 0;                                // indique si on est en mode AP ou non
bool flagSerial                = true;                             // to display once only

// ------ default values stored in PREFERENCE1  -------------------------------------------------------------------
constexpr char WIFI_SSID[]     = "Bureau";
constexpr char WIFI_PASS[]     = "enablemeplease";
constexpr int  VOLUME          = 10;
constexpr int  CHANEL          = 1;

WiFiMulti wifiMulti;
AsyncWebServer server(80);

Preferences preferences;

void setup() {
   initSerial();                                                    // Start Serial Monitor
   SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);                        // need to initialize SPI
   Serial.println("SPI initialized");
   initIOs();
   initLittleFS();
   init_screen();
   readPREF1();
   initWiFi();
   
   if (!modeAP) {                                                   // we have got an internet connection  
      initWebServer();                                              // init and start Webserver
      IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);        // IR initialization - default LED LED_BUILTIN (Red Input 02)
      ElegantOTA.begin(&server);                                    // Start ElegantOTA 
      printPREF2(maxStation);
      Serial.print("Stations array length : ");Serial.println(sizeof(stations));
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);     // init NTP time server, print time to serial and TFT display
      readTime();
      sprintf(NTPtime,"%02d:%02d:%02d",Heure,Minute,Seconde);       // format NTPtime  
      Serial.println(NTPtime);
      Message(NTPtime, 91, 2);
      delay(1500);     
      cadre_screen();                                               // clear screen and draw motif
      set_volume();
      set_station();                                                                                      
   }
}

void loop(){
   if (!modeAP){                                                   // only if not AP mode         
      audio.loop();                                        
      aff_time(8);
      check_buttons(); 
      check_IR();
      if ((millis()- t0_rssi) >= DELAY_RSSI){
         t0_rssi = millis();
         rssi = WiFi.RSSI();                                       // updates rssi every 2s (DELAY_RSSI)
      }
   }else{
      if(flagSerial){
         flagSerial = false;
         Serial.println("Waiting for WiFi credentials !");                // updates message every 2s (DELAY_RSSI)
      }  
   }
}

// Audio status functions 
void audio_info(const char *info) {
    AudioInfo = info;
    String sub =  AudioInfo.substring(0, 10);
    //Serial.print("audio_info: "); Serial.println(info);
    if (sub == "SampleRate") {
       AudioInfo = AudioInfo + " bps";
       aff_info(AudioInfo, 67);
    }
}

void audio_showstation(const char *info) {
    Station = info;
    //if (Station.length() <= 25) aff_info(Station, 58);
    //Serial.print("station     "); Serial.println(info);
}

void audio_showstreamtitle(const char *info) {
    Title = info;
    //if (Title.length() <= 25) aff_info(Station, 71);
    //Serial.print("streamtitle "); Serial.println(info);
}

void set_station() {   
    saveChanelToPREF1(cur_station);
    readCurStationFromPREF2(cur_station);
    aff_info(" ",67);                                              // clear sample rate display
    if ((Name != EMPTY) && (Url != EMPTY)) {
       audio.connecttohost(Url.c_str());                           // only connect if a correct url or name is set
       readVolumeFromPREF1();                                      // read current volume
       audio.setVolume(cur_volume);       
    }else{
       AudioInfo = "No url please add one";
       audio.setVolume(0);
       cur_volume = 0;
    }    
    Serial.print("New station set > ");Serial.print(Name); Serial.print(" url: "); Serial.println(Url);
    aff_name(cur_station, 40);
    aff_channel(107);
    delay(500);
}

void set_volume() {
    audio.setVolume(cur_volume);
    saveVolumeToPREF1(cur_volume);
    Serial.print("Volume: ");Serial.println(cur_volume);
    aff_volume(107);
}

void set_mute() {
    audio.setVolume(0);
    cur_volume = 0;
    Serial.print("Volume: ");Serial.println("Mute");
    aff_mute(107);
}

// ---------------------- Read Time and Date ----------------------------------------------------------------------------
void readTime() {                                                            
   struct tm timeinfo;
   if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time"); 
    return;
   }
   Heure = timeinfo.tm_hour;
   Minute = timeinfo.tm_min;
   Seconde = timeinfo.tm_sec;
   Annee = timeinfo.tm_year + 1900;
   Mois = timeinfo.tm_mon + 1;
   Jour = timeinfo.tm_mday;  
   byte dstOn = (31 - (5 * Annee / 4 + 4) % 7);                             // day in March that DST starts on, at 1 am
   byte dstOff = (31 - (5 * Annee / 4 + 1) % 7);                            // day in October that DST ends  on, at 2 am
   if ((Mois > 3 && Mois < 10) || (Mois == 3 && (Jour > dstOn || (Jour == dstOn && Heure >= 1))) || (Mois == 10 && (Jour < dstOff || (Jour == dstOff && Heure <= 1)))) {
      Heure = Heure + 1;                                                    // summer time - NTP est initialisé à GMT+1 (winter time)
      if (Heure == 24) Heure = 0;
   }        
}

// --------------------------------- Buttons management ------------------------------------------------
void check_buttons() {
   if (!digitalRead(CH_UP)) {                                                // button station+ pressed
    if (flag_chanUp == 0) {
        flag_chanUp = 1;
        t0_chanUp = millis(); 
        cur_station++;
        if (cur_station > maxStation) cur_station = MIN_STATION;
        set_station ();
    }else{
        if ( ( millis() - t0_chanUp) > DELAY_CHANEL) flag_chanUp = 0;
    }  
   }else if (!digitalRead(CH_DOWN)) {                                        // button station- pressed
    if (flag_chanDown == 0) {
        flag_chanDown = 1;
        t0_chanDown = millis(); 
        cur_station--;
        if (cur_station < MIN_STATION) cur_station = maxStation;
        set_station ();
    }else{
        if ( ( millis() - t0_chanDown) > DELAY_CHANEL) flag_chanDown = 0;
    }  
   }else if (!digitalRead(VOLUME_UP)) {                                      // button Volume+ pressed
     if (flag_volUp == 0) {
        flag_volUp = 1;
        t0_volUp = millis(); 
        if (flag_mute){
           readVolumeFromPREF1();
           set_volume();                                                     // last volume set
           flag_mute = 0;
           while(!digitalRead(VOLUME_UP));
        }else{
           cur_volume++;
        if (cur_volume > MAX_VOLUME) cur_volume = MAX_VOLUME;
          set_volume(); 
        }
     }else{
        if ( ( millis() - t0_volUp) > DELAY_VOLUME) flag_volUp = 0;
     }
   }else if (!digitalRead(VOLUME_DOWN)) {                                   // button station- pressed
     if (flag_volDown == 0) {
        flag_volDown = 1;
        t0_volDown = millis();     
        if (flag_mute){
          readVolumeFromPREF1();
          set_volume();                                                     // last volume set
          flag_mute = 0;
          while(!digitalRead(VOLUME_DOWN));
        }else{
          cur_volume--;
        if (cur_volume < MIN_VOLUME) cur_volume = MIN_VOLUME;
          set_volume();
        }      
     }else{
        if ( ( millis() - t0_volDown) > DELAY_VOLUME) flag_volDown = 0;
     }       
   }else if (!digitalRead(MUTE)) {                                         // button mute pressed
     if (flag_volMute == 0) {
        flag_volMute = 1;
        t0_volMute = millis();     
        if (flag_mute){
           readVolumeFromPREF1();
           set_volume();               // last volume set
           flag_mute = 0;
        }else{
           set_mute();
           flag_mute = 1;
        }
     }else{
        if ( ( millis() - t0_volMute) > DELAY_MUTE) flag_volMute = 0;
     }
       
   }
}

// ----------------------------------------- IR management -----------------------------------------------
void check_IR() {
   if (IrReceiver.decode()) {
    switch (IrReceiver.decodedIRData.command) {
      case CMD_NEXT_STATION:
       if (flag_chanIrUp == 0) {
         flag_chanIrUp = 1;
         t0_chanIrUp = millis(); 
         cur_station++;
         if (cur_station > maxStation) cur_station = MIN_STATION;
         set_station();
       }else{
         if ( ( millis() - t0_chanIrUp) > DELAY_CHANEL_IR) flag_chanIrUp = 0;
       }
        break;
      case CMD_PREV_STATION:
       if (flag_chanIrDown == 0) {
         flag_chanIrDown = 1;
         t0_chanIrDown = millis(); 
         cur_station--;
         if (cur_station < MIN_STATION) cur_station = maxStation;
         set_station();
       }else{
         if ( ( millis() - t0_chanIrDown) > DELAY_CHANEL_IR) flag_chanIrDown = 0;
       }
        break;
      case CMD_VOLUME_UP:
       if (flag_volIrUp == 0) {
        flag_volIrUp = 1;
        t0_volIrUp = millis(); 
        if (flag_mute){
           readVolumeFromPREF1();
           set_volume();               // last volume set
           flag_mute = 0;
           break;
        }
        cur_volume++;
        if (cur_volume > MAX_VOLUME) cur_volume = MAX_VOLUME;
        set_volume();
       }else{
         if ( ( millis() - t0_volIrUp) > DELAY_VOLUME_IR) flag_volIrUp = 0;
       }
        break;
      case CMD_VOLUME_DOWN:
       if (flag_volIrDown == 0) {
        flag_volIrDown = 1;
        t0_volIrDown = millis(); 
        if (flag_mute){
           readVolumeFromPREF1();
           set_volume();               // last volume set
           flag_mute = 0;
           break;
        }
        cur_volume--;
        if (cur_volume < MIN_VOLUME) cur_volume = MIN_VOLUME;
        set_volume();
       }else{
        if ( ( millis() - t0_volIrDown) > DELAY_VOLUME_IR) flag_volIrDown = 0;
       }
        break;
      case CMD_MUTE:
      case CMD_PAUSE:
       if (flag_volIrMute == 0) {
        flag_volIrMute = 1;
        t0_volIrMute = millis(); 
        if (flag_mute){
           readVolumeFromPREF1();
           set_volume();               // last volume set
           flag_mute = 0;
        }else{
           set_mute();
           flag_mute = 1;
        }
       }else{
         if ( ( millis() - t0_volIrMute) > DELAY_MUTE_IR) flag_volIrMute = 0;
       }
        break;
      case CMD_RESTART:
        WiFi.mode(WIFI_OFF);
        Serial.println("Rebooting...\n");
        tft.fillScreen(ST77XX_BLACK); 
        tft.setTextColor(ST77XX_RED);
        Message("Reboot !",55,2);
        Serial.flush();
        delay(1000);
        ESP.restart();
        break;
    }
    IrReceiver.resume();
  } 
}
