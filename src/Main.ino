#include<Arduino.h>

#include<U8g2lib.h>
#include<Keypad.h>
#include<DS3231.h>

#include<EEPROM.h>

#ifdef U8X8_HAVE_HW_SPI
#include<SPI.h>
#endif
//#ifdef U8X8_HAVE_HW_I2C
#include<Wire.h>
//#endif
#define SDExplorer 0x00
#define Reminder 0x01
#define Timer 0x02
#define Settings 0x03
#define MainMenu 0x04
#define KeyTestMenu 0x05
#define CritPSLoopDelay 0xC8 // 200
#define maxIndexOfSettingsOptions 0x03

// EEPROM Arrangement:
// eepromHeader: (0x0000, "#TT"); TinyTank will not use settings from EEPROM unless eepromHeader is present. 
// If not, all settings will be written to EEPROM with current settings while adding header to EEPROM.
// This process is called Formatting, though it will not write entire EEPROM.
// Brightness(Contrast): (0x0003, 0-255); 
// PowerSave Mode(PS): (0x0004, 0-2);
// PowerSave Mode is only active when currentMenu is MainMenu
// 0: No power saving
// 1: Displays topbar only after seconds(can be set by user, up to 255). Timer can be reset by pressing any buttom.
// 2: Turns on OLED power saving after seconds(can be set by user, up to 255). Timer can be reset by pressing any buttom.
// CritPS: This mode can only be CALLED from main menu. It's in fact a function, making TinyTank displays top bar only and turns on OLED power saving after only 5 sec. 
// Timer can be reset by pressing any buttom. TinyTank won't exit this mode until a reboot. Key detection loop delay will be extended to CritPSLoop miliseconds.

U8G2_SSD1309_128X64_NONAME0_2_4W_SW_SPI u8g2(U8G2_R2, /*clock*/ A0, /*data*/ A1, /*cs*/ A3, /*dc*/ A2);
const byte columnCounts = 4, rowCounts = 4;
const byte columnPins[columnCounts] = {6, 7, 8, 9}; // column 0-3
const byte rowPins[rowCounts] = {2, 3, 4, 5}; // row 0-3
const char keyList[columnCounts*rowCounts] = {
0x0A,0x07,0x08,0x09,
0x0B,0x04,0x05,0x06,
0x0C,0x01,0x02,0x03,
0x0D,0x0E,0x30,0x0F
};
Keypad keypad = Keypad(makeKeymap(keyList), rowPins, columnPins, rowCounts, columnCounts);

class Clock:public DS3231{
 public:
  Clock(){}
  byte decToBcd(byte val){
   return  ( (val/10*16) + (val%10) );
  }
  byte bcdToDec(byte val){
   return  ( (val/16*10) + (val%16) );
  }

  byte getAging(){
   Wire.beginTransmission(0x68);
   Wire.write(0x10);
   Wire.endTransmission();

   Wire.requestFrom(0x68, 1);
   return bcdToDec(Wire.read());
  }

  void setAging(byte offset){
   Wire.beginTransmission(0x68);
   Wire.write(0x10);
   Wire.write(decToBcd(offset));
   Wire.endTransmission();
  }
};

Clock clock;

bool eepromCheck = false;
bool PS = false;

bool needs_update_topbar = true;
bool needs_update_menu = true;
bool needs_draw_IME = false;
bool force_draw = true;

byte topbar_hour = 0, topbar_min = 0;
byte m[8] = {0};
byte currentMenu = MainMenu;
byte PSMode = 0x1;
byte Brightness = 0x80;
byte Delay = 0x64;

unsigned long PSTimeStamp = millis(), PSTime = 10000;

bool checkEepromFormat(){
 return EEPROM.read(0x0004) <= 0x2;
}

void setup(void) {
 Serial.begin(9600);
 keypad.setDebounceTime(100);
 Wire.begin();

 u8g2.begin();
 u8g2.setContrast(Brightness);
 
 u8g2.firstPage();
  
  // Small but a bit hard to read
  u8g2.setFont(u8g2_font_tom_thumb_4x6_mr);
  do{
  u8g2.drawStr(0, 6, "TinyTank Firmware DEMO by");
  u8g2.drawStr(0, 12, "FishBone Corporation");
  u8g2.drawStr(0, 18, "SD Reader Status...");
 } while(u8g2.nextPage());
 delay(500);
 u8g2.firstPage();
 do{
  u8g2.drawStr(76, 18, "coming soon");
 } while(u8g2.nextPage());
 u8g2.firstPage();
 do{
  u8g2.drawStr(0, 24, "EEPROM...");
 }while(u8g2.nextPage());

 char eepromHeader[3] = {0};
 EEPROM.get(0x0000, eepromHeader);

 if(eepromHeader == "#TT")
  eepromCheck = true;
 u8g2.firstPage();
 if(eepromCheck){
  if(checkEepromFormat()){
   do{
    u8g2.drawStr(76, 24, "Check");
   }while(u8g2.nextPage());
   u8g2.setContrast(EEPROM.read(0x0003));
   PSMode = EEPROM.read(0x0004);
  }else{
   do{
    u8g2.drawStr(76, 24, "Not Formatted");
   }while(u8g2.nextPage());
   eepromCheck = false;
  }
 }else{
  do{
   u8g2.drawStr(76, 24, "Header Absent");
  }while(u8g2.nextPage());
 }

 delay(1500);
 u8g2.firstPage();
}


void loop(void) {
  
 switch(currentMenu){
  case KeyTestMenu:
   update_KeyTestMenu();
   break;
  case MainMenu:
   update_MainMenu();
   break;
  case Settings:
   update_Settings();
   break;
  case Timer:
   update_Timer();
   break;
  default:
   update_ComingSoon();
 }
 update_topbar();

 // Picture Loop
 force_draw = needs_update_topbar || needs_update_menu || needs_draw_IME || force_draw;
 if(force_draw)
  u8g2.firstPage();
 do{
  if(needs_update_topbar || force_draw){
   draw_topbar();
  }
  if(needs_update_menu || force_draw){
   switch(currentMenu){
    case KeyTestMenu:
     draw_KeyTestMenu();
     break;
    case MainMenu:
     draw_MainMenu();
     break;
    case Settings:
     draw_Settings();
     break;
    case Timer:
     draw_Timer();
     break;
    default:
     draw_ComingSoon();
     break;
   }
  }
  if(needs_draw_IME)
   drawIME();
  
 }while(u8g2.nextPage());

 needs_update_topbar = false;
 needs_update_menu = false;
 needs_draw_IME = false;
 force_draw = false;

 delay((unsigned int)Delay);
}

void update_topbar(){
 if(PS && PSMode == 0x2)
  return;
 if(clock.getMinute() != topbar_min){
  needs_update_topbar = true;
  topbar_min = clock.getMinute();
 }else
  needs_update_topbar = false;
 return;
}

void update_Settings(){
 // General memory arrangement: 
 // 0: input key; 1: option index; 2: value digit index (0 for not editting this option, 1 for 1st digit); 3: value of 1st digit; 4: value of 2nd digit
 m[0] = keypad.getKey();
 if(!m[0])
  return;
 needs_update_menu = true;
 if(m[2]){
  m[2 + m[2]] = keyToHex();
  m[2]--;
  if(!m[2]){
   m[2] = m[3] + (m[4]<<4);
   switch(m[1]){
    case 0x00: //Brightness(Contrast)
     Brightness = m[2];
     u8g2.setContrast(Brightness);
     if(eepromCheck)
      EEPROM.update(0x0003, Brightness);
     break;
    case 0x01: //PSMode
     PSMode = m[2];
     if(eepromCheck)
      EEPROM.update(0x0004, PSMode);
     break;
    case 0x02: //Delay
     Delay = m[2];
     // TODO EEPROM update code here.
     break;
    case 0x03: //Aging Offset
     clock.setAging(m[2]);
     // TODO EEPROM update code here.
     break;
   }
   m[2] = 0x0;
  }
  return;
 }else{
  switch(m[0]){
   case 0x8:
    if(m[1])
     m[1]--;
    else
     m[1] = maxIndexOfSettingsOptions;
    break;
   case 0x2:
    if(m[1] == maxIndexOfSettingsOptions)
     m[1] = 0;
    else
     m[1]++;
    break;
   case 0x5:
   case 0x6:
    m[2] = 2;
    switch(m[1]){
     case 0x00:
      m[3] = Brightness;
      break;
     case 0x01:
      m[3] =  PSMode;
      break;
     case 0x02:
      m[3] = Delay;
      break;
     case 0x03:
      m[3] = clock.getAging();
      break;
    }
    m[4] = m[3] >> 4;
    m[3] &= 0xF;
    break;
   case 0xA:
    changeMenuTo(MainMenu);
    break;
   default:
    needs_update_menu = false;
    break;
  }
  return;
 }
}

void update_MainMenu(){
 // TODO Unify general memory arrangement
 m[1] = keypad.getKey();
 if(!m[1]){
  if(millis()-PSTimeStamp >= PSTime && !PS && PSMode){
   PS = true;
   switch(PSMode){
    case 0x1:
     clearMenu();
     break;
    case 0x2:
     u8g2.setPowerSave(true);
     break;
   }
  }
  return;
 }
 if(PS){
  force_draw = true;
  PS = false;
  PSTimeStamp = millis();
  if(PSMode == 0x2)
   u8g2.setPowerSave(false);
 }else{
  switch(m[1]){
   case 0x08: 
    if(m[0])
     m[0]--;
    else
     m[0] = 0x03;
    break;
   case 0x02:
    ++m[0] %= 4;
    break;
   case 0x05:
    changeMenu();
    break;
   default:
    return;
  }
  needs_update_menu = true;
 }
 return;
}

void update_ComingSoon(){
 m[1] = keypad.getKey();
 if(m[1] == 0x0A || m[1] == 0x05){
  changeMenuTo(MainMenu);
 }
 return;
}

void update_Timer(){ // TODO Optimization required.
 // NOTE: This piece of code is for Arduino UNO ONLY. Transplanting REQUIRED before any use for other boards.
 // 0: Input Key; 
 // 1: Mode (0 for standby, 1 for running, 2 for paused)
 // 2: Last second
 // 3: Flag for whether start time stamp and pause time stamp are allocated in memory
 // 4: 1st byte of address of array of time stamps
 // 5: 2st byte
 // In the array of time stamps...
 // [0]: Shift (Used by calculating past time by millis()-Shift
 // [1]: Pause
 // 6: displayed second, shortened to byte, used for updating
 // 7: current second, temporary use for updating
 // Note that all the time stamps are stored in unsigned long
 
 if(!m[3]){
  // Ensure that memory for time stamps has been allocated 
  // and addresses are stored
  Serial.print("Allocating memory for time stamps...");
  unsigned long *addr =  new unsigned long[2];
  addr[0]=addr[1]=0;
  (*(unsigned long **)(&m[4])) = addr;
  m[3] = 0x1;
  Serial.println("done");
 }

 m[0] = keypad.getKey();

 if(m[0] == 0x0A){
  Serial.print("Deleting time stamps memory...");
  delete[] *(unsigned long **)(&m[4]);
  Serial.println("done");
  changeMenuTo(MainMenu);
 }

 switch(m[1]){
  case 0: // Stanby
   switch(m[0]){ 
    case 0x05: // Standby, 5(Start)
     //((unsigned long*)(((m[4] & 0xFFFF) << 8) + m[5]))[0] = millis();
     (*(unsigned long **)(&m[4]))[0] = millis();
     m[1] = 1;
     needs_update_menu = true;
     return;
    default: // Stanby, no input
     return;
   }
   break;

  case 1: // Running
   if(m[0] == 0x05){
    (*(unsigned long **)(&m[4]))[1] = millis();
    //((unsigned long*)(((m[4] & 0xFFFF) << 8) + m[5]))[1] = millis();
    m[1] = 2;
    needs_update_menu = true;
    return;
   }
   m[7] = ((millis() - (*(unsigned long **)(&m[4]))[0]) / 1000) & 0xFF;
   if(m[7] != m[6]){
    m[6] = m[7];
    needs_update_menu = true;
   }
   return;

  case 2: // Paused
   if(m[0] != 0x05)
    return;
   (*(unsigned long **)(&m[4]))[0] += millis() - (*(unsigned long **)(&m[4]))[1];
   m[1] = 1;
   return;
 }
}
// (*(unsigned long *)(&m[4]))
// ((unsigned long*)(((m[4] & 0xFFFF) << 8) + m[5]))[0]

void update_KeyTestMenu(){
 // 0: int, Index of pointer.
 // 1: new key.
 // 2-7: char, characters;
 m[1] = keypad.getKey();
 if(m[1] != NO_KEY){
  Serial.print("new key:");
  Serial.println((char)m[1]);
  Serial.println(m[0]);
  m[2+m[0]++] = m[1];
  m[0] %= 6;
  Serial.println(m[0]);
  needs_update_menu = true;
  return;
 }
 needs_update_menu = false;
 return;
}


void draw_topbar(){
 if(PS && PSMode == 0x2)
  return;
 u8g2.drawLine(0, 7, 128, 7);
 u8g2.setCursor(0, 6);
 u8g2.print(clock.getTemperature(), 2);
 u8g2.print("C");
 u8g2.setCursor(112, 6);
 {
  bool b = false;
  u8g2.print(u8x8_u8toa(clock.getHour(b, b), 2));
 }
 u8g2.print(u8x8_u8toa(topbar_min, 2));
 return;
}

void draw_MainMenu(){
 if(PS)
  return;
 u8g2.drawStr(0, 13+6*SDExplorer, " SDExplorer");
 u8g2.drawStr(0, 13+6*Reminder, " Reminder");
 u8g2.drawStr(0, 13+6*Timer, " Timer");
 u8g2.drawStr(0, 13+6*Settings, " Settings");
 u8g2.drawStr(0, 13+6*m[0], ">");
 return;
}

void draw_Settings(){
  //TODO Optimize code structure
 const char* options[maxIndexOfSettingsOptions+1] = {" Brightness", " PSMode", " Delay", " Aging Offset"};
 drawText(0, 13, options, maxIndexOfSettingsOptions+1);
 if(m[2]){
  u8g2.setCursor(108, 13 + 6*m[1]);
  u8g2.print(">0x"); 
  u8g2.print(m[4], HEX);
  u8g2.print(m[3], HEX);
 }
 if(m[1] != 0x0 || !m[2]){
  u8g2.setCursor(112, 13);
  u8g2.print("0x");
  u8g2.print(Brightness>>4, HEX);
  u8g2.print(Brightness&0xF, HEX);
 }
 if(m[1] != 0x1 || !m[2]){
  u8g2.setCursor(112, 19);
  u8g2.print("0x");
  u8g2.print(PSMode>>4, HEX);
  u8g2.print(PSMode&0xF, HEX);
 }
 if(m[1] != 0x2 || !m[2]){
  u8g2.setCursor(112, 25);
  u8g2.print("0x");
  u8g2.print(Delay>>4, HEX);
  u8g2.print(Delay&0xF, HEX);
 }
 if(m[1] != 0x3 || !m[2]){
  u8g2.setCursor(112, 31);
  u8g2.print("0x");
  byte Aging = clock.getAging();
  u8g2.print(Aging>>4, HEX);
  u8g2.print(Aging&0xF, HEX);
 }
 if(!m[2])
  u8g2.drawStr(0, 13 + 6*m[1], ">");
 return;
}

void draw_ComingSoon(){
 u8g2.drawStr(31, 58, "Coming Soon!");
 return;
}

void draw_Timer(){
 unsigned long second;
 if(!m[1])
  second = 0;
 else
  second = (millis()-(*(unsigned long **)(&m[4]))[0])/1000;
 u8g2.setCursor(62, 31);
 u8g2.print(u8x8_u8toa(second/60, 2));
 u8g2.print(':');
 u8g2.print(u8x8_u8toa(second%60, 2));
 return;
}
// (*(unsigned long *)(&m[4]))
// ((unsigned long*)(((m[4] & 0xFFFF) << 8) + m[5]))[0]

void draw_KeyTestMenu(){
 u8g2.setCursor(4*m[0], 13);
 Serial.println(m[0]);
 u8g2.print((char)m[1]);
 return;
}

void drawIME(){
 u8g2.drawLine(0, 57, 127, 57);
 u8g2.setCursor(0, 63);
 for(byte i = 0x0; i< 0x10; i++){
  if(m[0]+i >= 0x20 && m[0]+i <= 0x7E){
   char string[2] = {m[0]+i, ' '};
   u8g2.print(string);
  }
  else
   u8g2.print("  ");
 }
}

void changeMenuTo(byte menu){
 m[0] = menu;
 changeMenu();
 return;
}

void changeMenu(){
 currentMenu = m[0];
 clearGeneralMem();
 clearMenu();
 force_draw = true;
 Serial.print("Menu changed to ");
 Serial.println(currentMenu);
 if(currentMenu == Timer){ // Remove this part after debugging
  update_Timer();
  Serial.println("update_Timer called");
 }
 return;
}

void drawText(byte x, byte y, char *string[], byte lineCount){
 for(byte i = 0; i < lineCount; i++){
  u8g2.drawStr(x, y+6*i, string[i]);
 }
 return;
}

void clearMenu(){
 // Note: Sets color to 0x1 after operation.
 u8g2.firstPage();
 do{
  u8g2.setDrawColor(0x0);
  u8g2.drawBox(0, 8, 128, 57);
  u8g2.setDrawColor(0x1);
  draw_topbar();
 }while(u8g2.nextPage());
 return;
}

byte keyToHex(){
 if(m[0] == 0x30)
  return 0x0;
 else return m[0];
}

String prefixTwoDigitHexWithZero(byte value){
 if(value >= 0x10)
  return String(value, HEX);
 else{
  return String(0) + String(value, HEX);
 }
}


void clearGeneralMem(){
 for(byte i = 0; i < 7; i++){
  m[i] = 0x0;
 }
 return;
}
