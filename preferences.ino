

void initPREF2() {
    String key1, key2;
    preferences.begin("Chanels", false);                             // read/write mode
    preferences.clear();                                             // clear all previous preferences
    maxStation = (sizeof(stations) / sizeof(stations[0]))/2;
    int i,j = 0;
    for (i = 1; i <= maxStation; i++) {
       key1 = "chanel_" + String(j);
       key2 = "chanel_" + String(j+1);
       String url = stations[j];
       String nom = stations[j+1];
       preferences.putString(key1.c_str(), url.c_str());
       preferences.putString(key2.c_str(), nom.c_str());
       j = j + 2;
    }
    for (i = maxStation+1; i <= MAX_STATION; i++) {
       key1 = "chanel_" + String(j);
       key2 = "chanel_" + String(j+1);
       preferences.putString(key1.c_str(), EMPTY);
       preferences.putString(key2.c_str(), EMPTY);  
       j = j + 2;
    }
    preferences.end(); 
    sortStations();                                                  // sort stations par name in NVS
    saveMaxStationToPREF1();
}

void printPREF2(int Max) {
    preferences.begin("Chanels", false);                             // read/write mode
    Serial.println("------------------------------------------------------------------------------------------------");
    int j = 0;
    for (int i = 1; i <= Max; i++) {
       String key1 = "chanel_" + String(j);
       String key2 = "chanel_" + String(j+1);
       String name = preferences.getString(key2.c_str() , "");
       for (int i = name.length(); i <= 12 ; i++) { 
          name = name + " ";
       }
       Serial.print(i<10 ? "0" + String(i)+ ">   " + name : String(i) + ">   " + name );
       Serial.print("\t");
       Serial.println("url : " + preferences.getString(key1.c_str() , "") );
       j = j + 2;
    }
    Serial.println("------------------------------------------------------------------------------------------------");
    preferences.end();
}

void readCurStationFromPREF2(byte station){
    preferences.begin("Chanels", false);                             // read/write mode
    String key1 = "chanel_" + String(2*station - 2);
    String key2 = "chanel_" + String(2*station - 1);
    Url = preferences.getString(key1.c_str(), EMPTY);
    Name = preferences.getString(key2.c_str(), EMPTY);
    preferences.end();
}

void initPREF1() {
    preferences.begin("currDefault", false);                         // read/write mode
    preferences.clear();                                             // clear all previous preferences
    preferences.putString("PREF_SSID", String(WIFI_SSID));
    preferences.putString("PREF_PWD", String(WIFI_PASS));
    preferences.putUChar("PREF_VOLUME", VOLUME);
    preferences.putUChar("PREF_CHANEL", CHANEL);
    preferences.end();
    Serial.println("Default Parameters restored");
}

void initPREF1_part() {
    preferences.begin("currDefault", false);                         // read/write mode - don't initialise WiFi credentials
    preferences.putUChar("PREF_VOLUME", VOLUME);
    preferences.putUChar("PREF_CHANEL", CHANEL);
    preferences.end();
    initPREF2();
    Serial.println("Default Parameters restored. New WiFi used");
}

void readPREF1(){
    if (!digitalRead(MUTE)) {                                        // init to default value if mute pressed at start up
       Serial.println("MUTE is pressed, restore default value");
       tft.fillScreen(ST77XX_BLACK);                                 // clear screen
       tft.setTextColor(ST77XX_RED);
       Message("Default Value", 45, 2);
       nvs_flash_erase();                                            // erase the NVS partition and...
       nvs_flash_init();                                             // initialize the NVS partition.
       delay(1000);
       initPREF1();
       initPREF2();
       Serial.println("Wait for MUTE to be released");
       tft.setTextColor(ST77XX_WHITE);
       Message("Please release MUTE !", 85,1);
       while(!digitalRead(MUTE));                                    // wait for menu is released  
       tft.fillScreen(ST77XX_BLACK);                                 // clear screen 
    }  
    preferences.begin("currDefault", false);
    ssid = preferences.getString("PREF_SSID", "");
    password = preferences.getString("PREF_PWD", "");
    cur_volume = preferences.getUChar("PREF_VOLUME" , 0);
    cur_station = preferences.getUChar("PREF_CHANEL", 0);
    maxStation = preferences.getUChar("PREF_MAX_CHAN", 0);
    preferences.end();
    Serial.printf("SSID: %s  Password: %s\n", ssid.c_str(), password.c_str());
    Serial.printf("Current Volume > %d\n", cur_volume);
    Serial.printf("Current Station > %d\n", cur_station);
    Serial.printf("Max stations > %d\n", maxStation);
}

// save new chanel to Preference 2
// --------------------------------
void saveNewChanelToPREF2(int chanel, String station, String url){
    preferences.begin("Chanels", false);                             // read/write mode
    String key1 = "chanel_" + String(2*chanel - 2);
    String key2 = "chanel_" + String(2*chanel - 1);
    preferences.putString(key1.c_str(), url);
    preferences.putString(key2.c_str(), station);  
    Serial.println("New chanel -> Has been stored in PREFERENCE 2");
    preferences.end();
}

// read Volume from Preference 1
// -------------------------------
void readVolumeFromPREF1() {
    preferences.begin("currDefault", false);                          // read/write mode
    cur_volume = preferences.getUChar("PREF_VOLUME", 0);
    preferences.end();
}

// Wifi SSID & PWD backup
// ------------------------
void saveWifiSetupToPREF1(String ssid, String pwd) {
    preferences.begin("currDefault", false);                          // read/write mode
    preferences.putString("PREF_SSID", ssid);
    preferences.putString("PREF_PWD", pwd);
    preferences.end();
    Serial.println("Wifi Setup -> Has been stored in PREFERENCE 1");
}

// Max Chanel backup
// -------------------------------
void saveMaxStationToPREF1() {
    preferences.begin("currDefault", false);                          // read/write mode
    preferences.putUChar("PREF_MAX_CHAN", maxStation);
    preferences.end();
}

// Chanel backup
// -------------------------------
void saveChanelToPREF1(byte chanel) {
    preferences.begin("currDefault", false);                          // read/write mode
    preferences.putUChar("PREF_CHANEL", chanel);
    preferences.end();
}

// Volume backup
// -------------------------------
void saveVolumeToPREF1(byte volume) {
    preferences.begin("currDefault", false);                          // read/write mode
    preferences.putUChar("PREF_VOLUME", volume);
    preferences.end();
}

// Sort stations
// -------------------------------
void sortStations() {
    String key1, key2;                                               // position of data and next data in Preference
    String temp1, temp2;                                             // temp1 = name/url and temp2 = next name/ next url
    preferences.begin("Chanels", false);                             // read/write mode
    for (int i = 0; i < maxStation - 1; i++) {                       // sort by alphabectic order using name
      for (int j = i + 1; j < maxStation; j++) {   
        key1 = "chanel_" + String(2*j+1);
        key2 = "chanel_" + String(2*i+1);
        temp1 = preferences.getString(key2.c_str(), EMPTY);  
        temp2 = preferences.getString(key1.c_str(), EMPTY);
        if (temp2 < temp1) {
           preferences.putString(key2.c_str(), temp2);               // exchange name
           preferences.putString(key1.c_str(), temp1);
           key1 = "chanel_" + String(2*j);
           key2 = "chanel_" + String(2*i);
           temp1 = preferences.getString(key2.c_str(), EMPTY);       // exchange url
           temp2 = preferences.getString(key1.c_str(), EMPTY);
           preferences.putString(key2.c_str(), temp2);
           preferences.putString(key1.c_str(), temp1);
        }    
      }
    }
    preferences.end();
}

// Delete station
// -------------------------------
void deleteStation(int rang) {
    String key1, key2;                                               // position of data and next data in Preference
    String temp1, temp2;                                             // temp1 = name/url and temp2 = next name/ next url
    preferences.begin("Chanels", false);                             // read/write mode
    for (int i = rang; i < maxStation ; i++) {                       // search if new name already exists in the list 
        key1 = "chanel_" + String(2*(i+1)+1);                        // name au rang + 1
        temp1 = preferences.getString(key1.c_str(), EMPTY);
        key1 = "chanel_" + String(2*i+1);                            // et on l'écrit au rang n
        preferences.putString(key1.c_str(), temp1);
        key2 = "chanel_" + String(2*(i+2));   
        temp2 = preferences.getString(key2.c_str(), EMPTY);          // url au rang n + 1
        key2 = "chanel_" + String(2*(i+1));
        preferences.putString(key2.c_str(), temp2);                  // et on l'écrit au rang n
    }
    preferences.end();
}

// Search same Name
// ---------------------------------------
void searchSameName(String station_new, String url_new) {
    preferences.begin("Chanels", false);                                           // read/write mode
    for (int i = 0; i < maxStation ; i++) {                                        // search if new name already exists in the list 
        String key1 = "chanel_" + String(2*i+1);   
        String temp = preferences.getString(key1.c_str(), EMPTY);
        String key2 = "chanel_" + String(2*i);   
        String temp2 = preferences.getString(key2.c_str(), EMPTY);
        if ((temp == station_new) && (temp2 == url_new)) {
           flagExist = true;
           break;
        }
    }
    preferences.end();                                                                 // read/write mode
}

// Set new chanel
// -------------------------------
void setNewChanel(String newName) {
    preferences.begin("Chanels", false);                             // read/write mode
    for (int i = 0; i < maxStation ; i++) {                          // search new chanel number after sorting
        String key1 = "chanel_" + String(2*i+1);   
        String temp = preferences.getString(key1.c_str(), EMPTY);
        if (temp == newName) {
           cur_station = i+1;
           set_station();
           break;
        }
    }
    preferences.end();
}
