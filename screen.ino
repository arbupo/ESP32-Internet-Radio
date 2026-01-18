
// ------------------------------------- ROUTINES de GESTION de l'ECRAN TFT ------------------------------------------

void init_screen() 
{
    tft.initR(INITR_BLACKTAB);                                       // Init ST7735S chip, black tab
    tft.fillScreen(ST77XX_BLACK);                                    // Clear screen
    tft.setRotation (3);                                             // Use landscape format (3 for upside down)
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    Message("ESP32 Radio",15,2);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    Message("(c)2023 Y.Bourdon",50,1);
    tft.setTextColor(ST77XX_GREEN);
    Message(Version,95,1); 
    tft.setTextColor(ST77XX_WHITE);
    delay(2000);
}

void cadre_screen()
{
    tft.fillScreen(ST77XX_BLACK);                                    // Clear screen
    tft.drawLine(0,0,screenWidth-1,0,ST77XX_WHITE);
    tft.drawLine(0,0,0,screenHeight-1,ST77XX_WHITE);
    tft.drawLine(screenWidth-1,0,screenWidth-1,screenHeight-1,ST77XX_WHITE);
    tft.drawLine(0,screenHeight-1,screenWidth-1,screenHeight-1,ST77XX_WHITE);
    tft.drawLine(1, 24, screenWidth-2, 24, ST77XX_WHITE);
    tft.drawLine(1, 93, screenWidth-2, 93, ST77XX_WHITE);
}

void aff_name(uint8_t station, uint8_t posY)
{
    uint8_t x;
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    String affName = Name;
    affName.trim();
    uint8_t l = affName.length();
    x = (screenWidth - l*11)/2 - 4;                                //character length = 5+1
    tft.fillRect(1,posY,screenWidth-2,16,0);                       //x,y,width,height,color
    tft.setCursor(x,posY);
    tft.println(affName); 
}

void aff_volume(uint8_t posY)
{
    char volumestring[10];
    uint8_t x;
    sprintf(volumestring,"Vol:%d/%d",cur_volume, MAX_VOLUME);
    x = (screenWidth - String(volumestring).length()*6)/2;        //character length = 5+1
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_GREEN);
    tft.fillRect(1,posY,screenWidth/2-8,8,0);                     //x,y,width,height,color
    tft.setCursor(10,posY);
    if (cur_volume != 0){
      tft.println(volumestring); 
    }else{
      tft.setTextColor(ST77XX_RED);
      tft.println("   MUTE");
      tft.setTextColor(ST77XX_GREEN);
    }
}

void aff_mute(uint8_t posY)
{
    uint8_t x;
    tft.setTextSize(1);
    tft.fillRect(1,posY,screenWidth/2-8,8,0);                     //x,y,width,height,color
    tft.setCursor(10,posY);
    tft.setTextColor(ST77XX_RED);
    tft.println("   MUTE");
    tft.setTextColor(ST77XX_GREEN);
}

void aff_channel(uint8_t posY)
{
    uint8_t x;
    char channelstring[20];                                             
    sprintf(channelstring,"Channel:%d/%d",cur_station,maxStation);
    x = (screenWidth - String(channelstring).length()*6)/2;         //character length = 5+1
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_GREEN);
    tft.fillRect(screenWidth/2-5,posY,screenWidth/2-1,8,0);         //x,y,width,height,color
    tft.setCursor(screenWidth/2-5,posY);
    tft.println(channelstring); 
}

void aff_time(uint8_t posY)    
{      
   readTime();
   if (Minute != Minute_old) {
      Minute_old = Minute;
      sprintf(NTPtime,"%02d/%02d/%04d %02d:%02d",Jour,Mois,Annee,Heure,Minute); //format NTPtime      
      tft.setTextColor(ST77XX_YELLOW);
      tft.setTextSize(1);
      uint8_t x = (screenWidth - String(NTPtime).length()*6)/2;    //character length = 5+1
      tft.fillRect(1,posY,screenWidth-x,8,0);                      //x,y,with,heigh - clear all line 
      tft.setCursor(x,posY);
      tft.println(NTPtime);  
   }
}

void aff_info(String info, int posY)
{
   uint8_t x;
   tft.setTextSize(1);
   tft.setTextColor(ST77XX_ORANGE);                                             
   x = (screenWidth - info.length()*6)/2;                           //character length = 5+1
   tft.fillRect(1,posY,screenWidth-2,8,0);//x,y,width,height,color
   tft.setCursor(x,posY);
   tft.println(info); 
}

void Message(String message, uint8_t posY, uint8_t size)
{
   tft.setTextSize(size);
   uint8_t x;
   x = (screenWidth - message.length()*6*size)/2;                  //character length = 5+1
   tft.fillRect(1,posY,screenWidth-x+2,8*size,0);                      //x,y,width,heigh - clear all line 
   tft.setCursor(x,posY);
   tft.println(message); 
}
