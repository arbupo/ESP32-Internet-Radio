
// ----------------------------------------------------------------------------
// Initialization procedures
// ----------------------------------------------------------------------------

// Serial monitor initialization
void initSerial() {
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("----------------------------------------------------------------------");
    Serial.println("                   Initialization process");
    Serial.println("----------------------------------------------------------------------");
}

// LED indicator initialization
void initIOs() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    digitalWrite(BLUE_LED, 0);                                                     // led off
    pinMode(CH_UP, INPUT_PULLUP);
    pinMode(CH_DOWN, INPUT_PULLUP); 
    pinMode(VOLUME_UP, INPUT_PULLUP);
    pinMode(VOLUME_DOWN, INPUT_PULLUP);
    pinMode(MUTE, INPUT_PULLUP);
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);                                  // Connect PCM5102A I2S Module
    Serial.println("1. I/O and I2S initialized");
}

// LittleFS initialization
void initLittleFS() {
   if(!LittleFS.begin(true)){
     Serial.println("An Error has occurred while mounting LittleFS");
     tft.setTextColor(ST77XX_RED);
     Message("LittleFS ko !", 65, 2);
     Message("Rebooting", 93, 2);
     Serial.flush();
     delay(2000);
     ESP.restart();
   }
   Serial.println("2. LittleFS volume is mounted");

   // List all files to confirm they exist
   Serial.println("Listing LittleFS files:");
   File root = LittleFS.open("/");
   File file = root.openNextFile();
   while(file){
       Serial.print("File: ");
       Serial.print(file.name());
       Serial.print("  Size: ");
       Serial.println(file.size());
       file = root.openNextFile();
   }
}

// WiFi connection initialization
void initWiFi() {
  String message;
  if(initWiFimode()) {
     modeAP = 0;
     Serial.print("\n5. Connected! => ");Serial.print(ssid);Serial.print("  "); Serial.println(WiFi.localIP());
     message = "Connected to : " + ssid;
     Message(message, 50, 1);
     message = WiFi.localIP().toString();
     rssi = WiFi.RSSI();
     Message(message, 65, 1);  
  }else{
     modeAP = 1;
     WiFi.mode(WIFI_AP);
     WiFi.softAP(APName, NULL);                                                         // No Password
     Serial.printf("\nFailed to connect. Starting access point, [%s]\n", APName);
     Serial.print("Connect to IP : ");Serial.print(WiFi.softAPIP());
     Serial.print("\n5. Connected in AP mode\n");
     Message("AP Mode", 55, 1);
     message = "Connect to " + String(APName);
     Message(message, 68, 1);
     message = WiFi.softAPIP().toString();;
     Message(message, 89,2);
     
     server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){                      // Web Server Root URL
     request->send(LittleFS, "/wifimanager.html", "text/html"); });                     
     server.serveStatic("/", LittleFS, "/");   
     server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
     int params = request->params();
     String firstSetup;
     for(int i=0;i<params;i++){
        const AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){ 
          if (p->name() == "ssid") {                                                   // HTTP POST ssid value
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);      
          }   
          if (p->name() == "pwd") {                                                    // HTTP POST pass value
            password = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(password);   
          } 
          if (p->name() == "init") {                                                   // HTTP POST pass value
            firstSetup = p->value().c_str();
            Serial.print("First Setup set to: ");
            Serial.println(firstSetup);   
          } 
        }
      }
      request->send(200, "text/plain", "ESP will restart");
      saveWifiSetupToPREF1(ssid, password); 
      if (firstSetup == "on")  initPREF1_part();                                       // init all but WiFi credentials
      reboot();
    });
    server.begin();
  }
}

// Test WiFi connexion
bool initWiFimode() {
  String message;
  int timeout = 15;
  modeAP = 0;
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.printf("4. Trying to connect to [%s] network ", ssid);
  tft.fillScreen(ST77XX_BLACK);                                                   // Clear screen
  Message("WiFi Setup", 15, 2);
  while(WiFi.status() != WL_CONNECTED) {
     Serial.print('.');
     delay(900);
     timeout--;
     message = String(timeout) + "/15";
     Message(message, 37, 1);
     if(timeout == 0) return false;
  }
  return true;
}
