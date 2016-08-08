/*
 Spot Welder uses  
 LiquidCrystal Library 
 for LCD Keypad Shield (http://www.icstation.com/product_info.php?products_id=1471#.UrbSAqFK_LA)

 The circuit for the shield:
 * LCD RS pin to digital pin 8
 * LCD Enable pin to digital pin 9
 * LCD D4 pin to digital pin 4
 * LCD D5 pin to digital pin 5
 * LCD D6 pin to digital pin 6
 * LCD D7 pin to digital pin 7
 * LCD R/W conencted to pin 11, but not used (to be checked)
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 Created by: Reinhard Nickels, Cologne, May 2015 
 This code is in the public domain.*/

#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

#define BL 10  // LCD Backlight
#define SS 3    // output to SolidState Relay
#define STARTLED 13
#define INT0 0  
#define INTPIN 2

#define WAIT_HW 100   // number of halfwaves to wait for stabilize

volatile unsigned int nd_count;  // counted zerocrossings, every count one halfwave, 10ms=50Hz 
unsigned long last_nd_count_time;  // for measuring of breaks between welds
unsigned long last_nd_count;   // buffer memory for historic nd_cont
unsigned long since_last_zc;     // zeit seit dem letzten zero crossing
volatile boolean zerocross_flag;  // should be reset with every zc

int key=-1;                    // keys of LCD shield -1=no, 0=rechts, 1=up ,2=down, 3=left , 4=select 
int oldkey=-1;
char screen[]= "-- DO  ---------------------------------P:01 B:05 M:007 ";
//             "0123456789012345------------------------B123456789012345";
int cursorpos=0;
volatile int menue=0;                 // starting with weld ready menue

volatile unsigned int pre_t, break_t, weld_t;       // Time constants as #of fullwave cycles(20ms), preWeld, break in between, weld
volatile boolean weld_done=false;
volatile boolean reset_nd_count=true;
unsigned long lastmillis;

void intHandler() {   // wird bei jedem Nulldurchgang aufgerufen
    if (menue==0 && !weld_done) {    
      if (reset_nd_count) {
        digitalWrite(STARTLED,HIGH);
        nd_count=0;
        reset_nd_count=false;
      }
      else {
        nd_count++;
        digitalWrite(STARTLED,LOW);
      }
      if (nd_count<=WAIT_HW) digitalWrite(SS,LOW);
      else if (nd_count<=WAIT_HW+pre_t) digitalWrite(SS,HIGH);   // pre weld
      else if (nd_count<=WAIT_HW+pre_t+break_t) digitalWrite(SS,LOW);   // break between
      else if (nd_count<=WAIT_HW+pre_t+break_t+weld_t) digitalWrite(SS,HIGH);
      else {
        digitalWrite(SS,LOW);
        weld_done=true;
      }
    }
}
void setup() {
  Serial.begin(115200);
  Serial.println("start...");
  // set up the LCD's number of columns and rows: 
  pinMode(SS,OUTPUT);
  digitalWrite(SS,LOW);
  pinMode(STARTLED, OUTPUT); 
  digitalWrite(STARTLED,LOW); 
  lcd.begin(16, 2);
//  lcd.blink();
  lcd.noCursor();
  // Print a message to the LCD.
  lcd.setCursor(0,0);
  lcd.print("Welcome to DF");
  lcd.setCursor(0,1);
  lcd.print("SpotWelder V1.0 ");
  delay(3000);

  lcd.setCursor(0,0);
  lcd.print(screen);
  pinMode(BL,OUTPUT);   // for backlight control
  analogWrite(BL,199);   //
  //  digitalWrite(BL, HIGH);}    // or switch
  
  pre_t=(screen[2+40]-48)*10+(screen[3+40]-48)<<1;
  break_t=(screen[7+40]-48)*10+(screen[8+40]-48)<<1;
  weld_t=(screen[12+40]-48)*100+(screen[13+40]-48)*10+(screen[14+40]-48)<<1;

  pinMode(INTPIN, INPUT_PULLUP);
  delay(100);  // warte bis alles stabil ist
  attachInterrupt(INT0, intHandler, FALLING);
  sei();
}

void loop() {
	
  lcd.setCursor(0,0);
  lcd.print(screen);
  lcd.setCursor(cursorpos,1);
  while (get_key()<0){
    key=-1;
    oldkey=-1;
    //  Check if startcondition
    if (millis()-lastmillis>1000) {
      Serial.print(weld_done);
      Serial.print(" ");
      Serial.print(nd_count);
      Serial.print(" ");
      Serial.print(last_nd_count_time);
      Serial.print(" ");
      Serial.print(last_nd_count);
      Serial.print(" ");
      Serial.print(pre_t);
      Serial.print(" ");
      Serial.print(break_t);
      Serial.print(" ");
      Serial.print(weld_t);
      Serial.println(" ");
      lastmillis=millis();
    }
    if (weld_done) {
      last_nd_count_time=millis();
      last_nd_count=nd_count;
      weld_done=false;
    }
    if (millis()-last_nd_count_time>100) {
      if (nd_count==last_nd_count) {
        nd_count=0;
        reset_nd_count=true;
      }
      else {
        last_nd_count_time=millis();
        last_nd_count=nd_count;
      }
    }
  }
  key = get_key();		        // convert into key press
  if (key != oldkey) {	                // if keypress is detected
    delay(50);		                // wait for debounce time
    key = get_key();		        // convert into key press
    if (key != oldkey) {			
      switch(key) {
      case 0:  // rechts
        if (menue==1) {
          if (cursorpos==15) cursorpos=0;
          else cursorpos++;
        }
        break;
      case 3:  // links
        if (menue==1) {
          if (cursorpos==0) cursorpos=15;
          else cursorpos--;
        }
        break;
      case 1:  // up
        if (menue==1) {
          if (screen[cursorpos+40]-48==9) screen[cursorpos+40]='0';
          else if (screen[cursorpos+40]-48>=0 && screen[cursorpos+40]-48<9) screen[cursorpos+40]++;
        }
        break;
      case 2:  // down
        if (menue==1) {
          if (screen[cursorpos+40]-48==0) screen[cursorpos+40]='9';
          else if (screen[cursorpos+40]-48>0 && screen[cursorpos+40]-48<=9) screen[cursorpos+40]--;
        }
        break;
      case 4:  // select
        if (menue==1) {
          screen[3]= 'D';
          screen[4]= 'O';
          screen[5]= ' ';
          menue=0;
          cursorpos=0;
          lcd.noCursor();
          pre_t=(screen[2+40]-48)*10+(screen[3+40]-48)<<1;
          break_t=(screen[7+40]-48)*10+(screen[8+40]-48)<<1;
          weld_t=(screen[12+40]-48)*100+(screen[13+40]-48)*10+(screen[14+40]-48)<<1;
          Serial.println("Anzahl Nulldurchgaenge");
          Serial.print(pre_t);Serial.print(" ");
          Serial.print(break_t);Serial.print(" ");
          Serial.print(weld_t);Serial.println(" ");
          nd_count=0;
        }
        else {
          screen[3]= 'S';
          screen[4]= 'E';
          screen[5]= 'T';
          menue=1;
          cursorpos=2;
          lcd.cursor();
        }
      
      break;
      }
      oldkey = key;
    }
  }
}

// Convert ADC value to key number
int get_key()
{
  unsigned int adc_key_in = analogRead(0);    // read the value from the sensor  
  int NUM_KEYS = 5;
  int  adc_key_val[5] ={
    30, 150, 360, 535, 760   };
  int k;
  for (k = 0; k < NUM_KEYS; k++)
  {
    if (adc_key_in < adc_key_val[k])
    {
      return k;
    }
  }
  if (k >= NUM_KEYS)
    k = -1;     // No valid key pressed
  return k;
}




