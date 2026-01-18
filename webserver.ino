
// ----------------------------------------------------------------------------
// HTTP route definition & request processing
// ----------------------------------------------------------------------------

// Definition of request handlers and server initialization
// --------------------------------------------------------
void initWebServer() {
  // Routes that correspond to dynamic processing by the microcontroller:
  server.on("/",           onRootRequest);
  server.on("/dataUpdate", onDataUpdate);                                           // appelé par JS toutes les 1000mS
  server.on("/reset",      onReset);
  server.on("/reboot",     onReboot);
  server.on("/setwifi",    onSetWifi);                                              // set wifi parameters after Save pressed
  server.on("/setchanel",  onSetChanel);                                            // set chanel parameters after Save pressed
  server.on("/setsel",     onSetSel);                                               // set volume and chanel parameters after OK pressed
  server.onNotFound(onNotFound);
  server.serveStatic("/", LittleFS, "/");                                           // LittleFS is used to store html, css, js and favicon
  server.begin();                                                                   // server initialization
  Serial.println("6. Web server started");
}

// Processing of the `\parametres.html` template                                    // premier accès valeurs initiales
// ---------------------------------------------
String ParametresProcessor(const String &var)
{
  if (var == "VERSION") {
    return (String(Version));
  } else if (var == "HEURE") {                                                      // time and date
    readTime();
    sprintf(NTPtime, "%02d/%02d/%04d %02d:%02d", Jour, Mois, Annee, Heure, Minute); // format NTPtime
    return String(NTPtime);
  } else if (var == "WIFI_SSID") {                                                  // Wifi SSID
    return String(ssid);
  } else if (var == "WIFI_PWD") {                                                   // Wifi Password
    return String(password);
  } else if (var == "WIFI_RSSI") {                                                  // WIFI SSID de la connexion active (mesurée une seule fois à la connexion)
    return String(rssi);
  } else if (var == "VOLUME") {                                                     // current volume
    return String(cur_volume);
  } else if (var == "CUR_STATION") {                                                // current station
    return String(cur_station);
  } else if (var == "CUR_CHANEL") {                                                 // current name of chanel
    return Name;
  } else if (var == "CUR_URL") {                                                    // current url of chanel
    return Url;
  } else if (var == "MAX_STA") {                                                    // max number of chanel
    return String(MAX_STATION);
  } else if (var == "NEXT_STA") {                                                   // next avalable station
    return String(maxStation+1);
  } else if (var == "CHANEL") {                                                     // maxStation stored
    return String(maxStation);
  } else if (var == "NAME") {                                                       // new name for chanel xx
    return NEW_NAME;
  } else if (var == "URL") {                                                        // new url for chanel xx
    return NEW_URL;
  }
  return String();
}

// Specific treatment of the root page (as a template)
// ---------------------------------------------------
void onRootRequest(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/parametres.html", "text/html", false, ParametresProcessor);
}

// Manager for queries of values set by the operator
// -------------------------------------------------
void onSetWifi(AsyncWebServerRequest *request) {
  bool flagReboot = 0;
  if ( request->hasParam("ssid") && request->hasParam("pwd") ) {
    String ssid_new = request->getParam("ssid")->value();
    String pwd_new = request->getParam("pwd")->value();
    if ((ssid_new != ssid) || (pwd_new != password)) {                             // nouveau SSID ou PWD
      Serial.printf("Wifi SSID received: %s\n", ssid_new);
      saveWifiSetupToPREF1(ssid_new, password);
      request->send(200);                                                          // requests are asynchronous and must always be resolved:
      flagReboot = 1;
    }
    if (pwd_new != password) {                                                     // nouveau SSID ou PWD
      Serial.printf("Wifi Password received: %s\n", pwd_new);
      saveWifiSetupToPREF1(ssid, pwd_new);
      request->send(200);                                                          // requests are asynchronous and must always be resolved:
      flagReboot = 1;
    }
    if (flagReboot == 1) reboot();                                                 // nouveau ssid, pwd, IP, IP gateway -> reboot
  }
  request->send(200);                                                              // requests are asynchronous and must always be resolved:
}

// Manager for queries of values set by the operator
// -------------------------------------------------
void onSetChanel(AsyncWebServerRequest *request) {
  if ( request->hasParam("chanel") && request->hasParam("station") && request->hasParam("urlStation") ) {
    audio.setVolume(0);
    delay(200);
    int chanel_new = request->getParam("chanel")->value().toInt();
    String station_new = request->getParam("station")->value();
    String url_new = request->getParam("urlStation")->value();
    bool flagExist = false;
    searchSameName(station_new,url_new);
    if (flagExist == true) Serial.println(station_new + " is already in the list");
    Serial.printf("New chanel received: %d\n", chanel_new);
    Serial.print("New name received: "); Serial.println(station_new);
    Serial.print("New url received: "); Serial.println(url_new);
    if ((flagExist == false) && (station_new == DELETE) && (maxStation >= 1) && (chanel_new <= maxStation) ){        // delete station and decrements maxStation
       maxStation = maxStation - 1;
       saveMaxStationToPREF1();
       Serial.println("Delete Station");
       Serial.print("maxStation has been updated: "); Serial.println(maxStation);
       deleteStation(chanel_new - 1);                                                     // shifts the array down one row
       Serial.println("Ok done !");
       printPREF2(maxStation);
       if (chanel_new >  maxStation){
          cur_station = maxStation;
       }else{
          cur_station = chanel_new;
       }
       set_station();
    }
    if ( (chanel_new <= maxStation + 1) && (station_new != NEW_NAME) && (station_new != DELETE) && (url_new != NEW_URL) && (flagExist == false)){
       if (chanel_new > maxStation) {
         maxStation = chanel_new;
         saveMaxStationToPREF1();
         Serial.print("maxStation has been updated: "); Serial.println(maxStation);
       }
       saveNewChanelToPREF2(chanel_new, station_new, url_new);
       Serial.println("New chanel and url have been saved to Preference 2");
       Serial.print("Sort Stations ...  ");
       sortStations();
       Serial.println("Ok done !");
       printPREF2(maxStation);
       setNewChanel(station_new);
    }
    audio.setVolume(cur_volume);
  }
  request->send(200);                                                               // requests are asynchronous and must always be resolved:
}

// Manager for queries of values set by the operator
// -------------------------------------------------
void onSetSel(AsyncWebServerRequest *request) {
  if ( request->hasParam("selChan") && request->hasParam("selVol") ) {
    audio.setVolume(0);
    delay(500);
    int chanel_new = request->getParam("selChan")->value().toInt();
    int volume_new = request->getParam("selVol")->value().toInt();
    Serial.printf("New chanel received: %d\n", chanel_new);
    Serial.printf("New volume received: %d\n", volume_new);
    if ( (chanel_new <= maxStation) && (chanel_new != cur_station) ) {
      saveChanelToPREF1(chanel_new);
      Serial.println("New chanel has been saved to Preference 1");
      cur_station = chanel_new;
      
      set_station();
    }
    if ( (volume_new <= MAX_VOLUME) && (volume_new != cur_volume) ) {
      saveVolumeToPREF1(volume_new);
      Serial.println("New volume has been saved to Preference 1");
      cur_volume = volume_new;
      set_volume();
    }
  }
  request->send(200);                                                                // requests are asynchronous and must always be resolved:
}


// Method of fallback in case no request could be resolved
// -------------------------------------------------------
void onNotFound(AsyncWebServerRequest *request) {
  request->send(404);
}

// Time and data reading query manager
// ---------------------------------------------
void onDataUpdate(AsyncWebServerRequest *request) {                                   // appelé par JS toutes les x mS (actuellement 800mS)
  readTime();
  String answer = String( NTPtime) + "|" + String("Volume: ") + String(cur_volume) + "|" + String(cur_station) + String("/") + String(maxStation) + "|" +  Name + "|" + Url  + "|" + String(rssi) + "|" + String(maxStation+1);
  request->send(200, "text/plain", answer);
}

// Factory reset
// -------------
void onReset(AsyncWebServerRequest *request) {
  request->send(200);                                                               // requests are asynchronous and must always be resolved
  audio.setVolume(0);
  delay(500);
  Serial.println("Restore default value");
  tft.fillScreen(ST77XX_BLACK);                                                     // clear screen
  tft.setTextColor(ST77XX_RED);
  Message("Default Value", 55, 2);
  tft.setTextColor(ST77XX_WHITE);
  nvs_flash_erase();                                                                // erase the NVS partition and...
  nvs_flash_init();                                                                 // initialize the NVS partition.
  delay(1000);
  initPREF1();
  initPREF2();
  ESP.restart();
}

// ESP32 restart request manager
// -----------------------------
void onReboot(AsyncWebServerRequest *request) {
  request->send(200);                                                               // requests are asynchronous and must always be resolved
  audio.setVolume(0);
  delay(500);
  Serial.println("------------------------------------------");
  reboot();
}

// -------------------------- reboot ESP32 -------------------------------
// -----------------------------------------------------------------------
void reboot() {
  audio.setVolume(0);
  delay(500);
  WiFi.mode(WIFI_OFF);
  Serial.println("Rebooting...\n");
  Serial.flush();
  delay(500);
  ESP.restart();
}
