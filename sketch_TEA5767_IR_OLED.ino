/*--------------------------------------------------------------
TEA5767 FM Radio
By Chris Rouse Jan 2016

Radio Functions:
* preset station selection
* mute/unmute radio
* select favourite station
* shows if station is being received in Stereo or Mono
* shows signal strength
* 
* Keys required on IR controller::
* presetUP - next preset station
* presetDOWN - next preset station down
* radioON_OFF - turns radio ON or OFF
* favoutiteSTATION - select your favourite station
* scan - scans through FM spectrum looking for stations
* finetuneUP - increments frequency up by 0.1mHz
* finetuneDOWN - lowers frequency by 0.1mHz increments
* 


Using Arduino Uno 

Up to 6 preset stations can be selected although this can easily be increased
SELECT switches to your favourite station

Tuning is in 0.01mHz intervals to allow for fine tuning of presets

There are 4 pins to connect, SDA to Arduino Pin A4, SCL to Arduino pin A5 and Vcc to 5 volts, Gnd to Gnd
or if using Arduino Mega SDA (pin 20), SCL (pin 21), 5volts and Gnd (power available on end socket). Plug the LCD Shield
 ---------------------------------------------------------------*/

/*--------------------------------------------------------------
* Setup OLED Display
---------------------------------------------------------------*/
#include "U8glib.h"
#include <SPI.h>
// setup u8g object
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);  // I2C 
//
/*--------------------------------------------------------------
* Setup Radio
---------------------------------------------------------------*/
#include <Wire.h>
#include <TEA5767N.h>
// Init radio object
TEA5767N radio = TEA5767N(); 
/*------------------------------------------------------------- 
 *  Setup IR Receiver
---------------------------------------------------------------*/
#include <IRremote.h>
int RECV_PIN = 2;
int LED_PIN = 13;
IRrecv irrecv(RECV_PIN);
decode_results results;
// these are the values for the IR contro; buttons
// alter to suit your IR setup, these are DECIMAL values
//
const double radioON_OFF = 16580863;
const double favouriteSTATION = 16597183;
const double presetUP = 16613503;
const double presetDOWN = 16617583;
const double finetuneUP = 16601263;
const double finetuneDOWN = 16584943;
//
/*--------------------------------------------------------------------------------------
Variables
--------------------------------------------------------------------------------------*/

double frequency = 88.00; // frequency in mHz
double topFrequency = 110.00;
double bottomFrequency = 88.00;
double intervalFreq = 0.01; // could be changed to 1 for coarse tuning
// alter to add more or less presets
int maxPresets; // maximum number of presets used
double stationsMHZ[7]; // allow for 6 preset stations
String stations[7]; // station names
//
int presetCounter = 0; //pointer to preset stations
int favourite; // pressing SELECT will select this one
int sl; // signal level 0 - 10
boolean mute = true;
String stringFrequency;
String stringStation;
String stringSignal;
boolean stereo= false;
String rawData;
boolean IRsignal = false;
int a;
boolean stationName = true; // display the station name
/*--------------------------------------------------------------------------------------
Setup
--------------------------------------------------------------------------------------*/
void setup()
{
  Wire.begin();
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the IR receiver
  radio.selectFrequency(bottomFrequency); // set radio initially to lowest frequency
  pinMode(3,INPUT_PULLUP); // this pin controls the setup screen
  digitalWrite(3, HIGH);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  // setup preset list of stations
  // for the character set used
  // names must be less than 21 characters
  stationsMHZ[1] = 88.58;
  stations[1] = "BBC Radio 2"; 
  stationsMHZ[2] = 96.10;
  stations[2] = "Radio Solent"; 
  stationsMHZ[3] = 97.50;
  stations[3] = "Heart Solent"; 
  stationsMHZ[4] = 98.20;
  stations[4] = "BBC Radio 1"; 
  stationsMHZ[5] = 103.20;
  stations[5] = "Capital FM"; 
  stationsMHZ[6] = 102.00;
  stations[6] = "Isle of Wight Radio";
  //
  maxPresets = 6; // number of stations in list, change this as required
  favourite = 6; // select your own favourite  
  // ensure radio starts with favourite
  presetCounter = favourite;
  selectStation();

}
/*--------------------------------------------------------------------------------------
Main Loop
--------------------------------------------------------------------------------------*/
void loop() {
  // if pin 3 is connected to the Gnd pin then just display output from IR Receiver
  if(digitalRead(3) == 0){
    // picture loop
    u8g.firstPage();  
    do {
      drawSetupCalibrate();
    } while( u8g.nextPage() );
    calibrateIR();
  }
  //otherwise we are in the Radio  receiver mode
  // Decode commands from the IR Remote
  if (irrecv.decode(&results)) {
      digitalWrite(LED_PIN, HIGH);
    // this selects the next radio station
    if(results.value == presetUP){
      presetCounter = presetCounter + 1;
      if(presetCounter > maxPresets){
        presetCounter = 1; // loop counter round
      }
      stationName = true;
      selectStation();
      }
    
    // this selects the last preset radio station
    if(results.value == presetDOWN){
      presetCounter = presetCounter - 1;
      if(presetCounter <1){
        presetCounter = maxPresets; // loop counter round
      }
      stationName = true;
      selectStation();
    }
  
    // this selects your favourite radio station
    if(results.value == favouriteSTATION){
      if (favourite > 0 and favourite < maxPresets + 1) {
        presetCounter = favourite;
        frequency = stationsMHZ[presetCounter];
        stationName = true;
        selectStation();
      }
    }
    
    // this section mutes or unmutes the radio
    if(results.value == radioON_OFF){
      if(mute){
        radio.mute();
        radio.setStandByOff();
      }
      else{
        radio.turnTheSoundBackOn();
      }
      mute = !mute;
    }

    // this section increases the frequency to allow fine tuning
    if(results.value == finetuneUP){
      frequency = frequency + intervalFreq;
      if(frequency > topFrequency){
        frequency = bottomFrequency; // reset to lowest frequency 
      }
      stationName = false; // dont display the station name
      selectStation2();
    } 
       
    // this section decreases the frequency to allow fine tuning
    if(results.value == finetuneDOWN){
      frequency = frequency - intervalFreq;
      if(frequency < bottomFrequency){
        frequency = topFrequency; // reset to highest frequency 
      }
      stationName = false; // dont display the station name
      selectStation2();
    }    
    // end of button press checks
    //
    irrecv.resume(); // Receive the next value
    digitalWrite(LED_PIN, LOW); // turn off onboard LED
    //
  }
  // get Stereo status and signal strength
  printStereoStatus();
  // picture loop
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );
} // end Main loop


/*--------------------------------------------------------------------------------------
Get frequency for a station in the preset list
--------------------------------------------------------------------------------------*/
void selectStation() {
  frequency = stationsMHZ[presetCounter];
  radio.mute();
  radio.selectFrequency(frequency); // set radio frequency
  radio.turnTheSoundBackOn();
}
/*--------------------------------------------------------------------------------------
* Send a frequency to the TEA 5767
--------------------------------------------------------------------------------------*/
void selectStation2() {
  radio.mute();
  radio.selectFrequency(frequency); // set radio frequency
  radio.turnTheSoundBackOn();
}
/*--------------------------------------------------------------------------------------
* Get Stereo Status
--------------------------------------------------------------------------------------*/
void printStereoStatus() {
  sl = radio.getSignalLevel();
  if (radio.isStereo()) {
    stereo = true;
  } else {
    stereo = false;
  }
}
/*--------------------------------------------------------------------------------------
Draw display
--------------------------------------------------------------------------------------*/
void draw(void){
  u8g.setFont(u8g_font_profont12); 
  u8g.drawStr(40,8, "FM Radio");
  if(mute){ // radio is muted
    // display station frequency  
    stringFrequency = String(radio.readFrequencyInMHz()); // display actual frequency
    stringFrequency = stringFrequency + "mhz";
    a = stringFrequency.length();
    a = 18 - a;  
    a = a*7; // width in pixels of empty space    
    a = (a/2) + 1; // start for string to make it centre 
    const char* newFrequency = (const char*) stringFrequency.c_str();
    u8g.setFont(u8g_font_profont15); 
    u8g.drawStr(a,30, newFrequency);
    // display station title 
    u8g.setFont(u8g_font_profont12);
    if (stationName){
     stringStation = stations[presetCounter]; 
    }
    else{stringStation = "fine tune";}
    a = stringStation.length();
    a = 21 - a;  
    a = a*6; // width in pixels of empty space    
    a = (a/2) + 1; // start for string to make it centre 
    const char* newStation = (const char*) stringStation.c_str();
    u8g.drawStr(a,42,newStation);
    // display signal strength and Stereo/Mono
    if(stereo){
      u8g.drawStr(5,17,"Stereo");
    }
    else{
      u8g.drawStr(5,17,"Mono");
    }
    if(sl <10){
      stringSignal = "Signal: " + String(sl);
      const char* newSignal = (const char*) stringSignal.c_str();
      u8g.drawStr(70,17,newSignal);
    }

    // now draw frequency line
    u8g.drawLine(4,58,124,58);
    for(int f = 4; f<128; f=f+40){
      u8g.drawLine(f,52,f,58);
    }
    for(int f = 4; f<128; f=f+8){
      u8g.drawLine(f,55,f,58);
    }   
    u8g.drawLine(((frequency-80)*4)+4,53,((frequency-80)*4)+4,63);
    u8g.setFont(u8g_font_profont10);
    u8g.drawStr(0,52,"80"); 
    u8g.drawStr(40,52,"90");
    u8g.drawStr(78,52,"100");
    u8g.drawStr(114,52,"110"); 
  }
  else{
    u8g.drawStr(23,30,"Radio is muted");
  }
  
}
/*--------------------------------------------------------------------------------------
display the values sent by the IR control
--------------------------------------------------------------------------------------*/
void calibrateIR(){
  // just display values sent by the IR control
  while(digitalRead(3) == 0){
    if (irrecv.decode(&results)) {
      digitalWrite(LED_PIN, HIGH); // onboard LED shows IR signal received
      // picture loop
      u8g.firstPage();  
      do {
        drawSetup();
      } while( u8g.nextPage() );
    }
    delay(500);
    digitalWrite(LED_PIN, LOW);
  }
}
/*--------------------------------------------------------------------------------------
Draw display to show data from IR controller
--------------------------------------------------------------------------------------*/
void drawSetup(){
  radio.mute(); // turn off the radio to reduce noise
  u8g.setFont(u8g_font_profont12); 
  u8g.drawStr(15,8, "Codes sent by IR");
  u8g.drawLine(0,15,128,15);
  u8g.drawLine(0,55,128,55); 
  // now print code
  rawData = String(results.value);
  const char* newRawData = (const char*) rawData.c_str();
  u8g.setFont(u8g_font_profont15);
  u8g.drawStr(35,40,newRawData);
  irrecv.resume(); // Receive the next value
}
void drawSetupCalibrate(){
  u8g.setFont(u8g_font_profont12); 
  u8g.drawStr(15,8, "Codes sent by IR");
  u8g.drawStr(20,40,"Wating for Data");
  u8g.drawLine(0,15,128,15);
  u8g.drawLine(0,55,128,55); 
}
/*--------------------------------------------------------------------------------------*/
