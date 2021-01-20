#include <DHT.h>
#include <DS3231.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <time.h>
#include <stdio.h>
//#include <stdlib.h>
//#include <util/eu_dst.h>
#include <Arduino.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "Adafruit_BLEEddystone.h"
#include "BluefruitConfig.h"
#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif


// macros that convert the String obtained from preprocessor macros __DATE__ or __TIME__ into byte in order to set the clock of DS3231
// __DATE__ format: MMM DD YYYY 
// __TIME__ format: HH:MM:SS 
// for more information see  DS3231.cpp 
#define BuildDay ((__DATE__[4]-'0') * 10 +(__DATE__[5]-'0')) 
#define BuildYear ((__DATE__[9]-'0') * 10 +(__DATE__[10]-'0')) 
#define BuildHour ((__TIME__[0]-'0') * 10 +(__TIME__[1]-'0')) 
#define BuildMinute ((__TIME__[3]-'0') * 10 +(__TIME__[4]-'0')) 
#define BuildSecond ((__TIME__[6]-'0') * 10 +(__TIME__[7]-'0')) 

#define DHTTYPE DHT22
#define DHTPIN 22 
#define SD_CS 23 
//#define DEBUG
//#define SET_TIME

DS3231 Clock;
DHT dht(DHTPIN, DHTTYPE);
File dataFile;

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_BLEEddystone eddyBeacon(ble);


bool Century;
bool PM;
bool h12;
char fileName[] = "00000000.txt";

// init the struct of our datalog 
struct DataLog 
{
  float temperature;
  float humidity;
  DateTime current_date;
  //float gas_concentration;
  
} data;

float ohm_to_concentration(float ohm){

  //...... conversion code

}

// this function simply converts time from the DateTime format used by the DS3231 sensor (view DS3231.h file)
// into a string that will be saved into the SD file 

char TimeToString(DateTime date, char *shortDate){

  
  // we use the format HH:MM:SS
  snprintf(shortDate,sizeof(char)*9,"%02d:%02d:%02d",date.hour(),date.minute(),date.second());
  
  
}

// this function does the same as the other above with the difference that here is the data that is being converts into string
void DateToString(DateTime date, char *shortDate){

  
  // we use the format YYYY/MM/DD
  snprintf(shortDate,sizeof(char)*11,"%04d/%02d/%02d",date.year(),date.month(),date.day());
  
  
}

// this function create the final string that will be stored into the file on SD card
void create_data_log_string(DataLog data, char *dataString){
 
    char time_[9];
    TimeToString(data.current_date,time_);
    
    snprintf(dataString,sizeof(char)*52,"Temperatura: %d.%d °C Umidità: %d.%d  Ora: %s",(int)data.temperature,((int)(data.temperature*10))%10,(int)data.humidity,((int)(data.humidity*10))%10,time_);
   
}

void initSDcard(){
  Serial.print(F("Initializing SD card..."));

  // see if the card is present and can be initialized:
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
    // don't do anything more:
    while (1);
  }
  Serial.println(F("card initialized."));
}


// this function converts the month String obtained through the __DATE__ macro into numbers from 1 to 12 
byte month_number(char string[]){
  byte month;
  switch(string[0]){

    case 'J':
      if(string[1] == 'a')
        month =  1;
      else if(string[2] == 'n')
        month = 6;
      else month = 7;
    break;
    
    case 'F' :
      month = 2;
    break;

    case 'M' :
      if(string[2] == 'r')
        month = 3;
      else
        month = 5;
    break;

    case 'A' :
      if(string[1] == 'p')
        month = 4;
      else 
        month = 8;
    break;

    case 'S' :
      month = 9;
    break;

    case 'O' :
      month = 10;
    break;

    case 'N' : 
      month = 11;
    break;

    case 'D' :
      month = 12;
    break;

    default:
      Serial.println("Error recognizing month, check if system language is English"); 
    break;
  
  }
  return month;
}

// this function has to be called only one time into the setup() in order to set the time of the DS3231 module with the system time at compile
void setDS3231time(){
  // __DATE__  give the time at the compilation of the code
  char date_now[] = __DATE__;
  
  
  byte year = BuildYear;  
  byte month = month_number(date_now);
  byte day = BuildDay; 
  byte hour= BuildHour; 
  byte minute = BuildMinute;   
  byte second = BuildSecond;  
  
  // for more information about this function see DS3231.h
  Clock.setClockMode(false);
  Clock.setYear(year);
  Clock.setMonth(month);
  Clock.setDate(day);
  Clock.setHour(hour);
  Clock.setMinute(minute);
  Clock.setSecond(second);

}

DateTime readDS3231date(){
  return  DateTime(
    (uint16_t)Clock.getYear() + 2000 ,
    (uint8_t)Clock.getMonth(Century) ,
    (uint8_t)Clock.getDate() ,
    (uint8_t)Clock.getHour(h12,PM) ,
    (uint8_t)Clock.getMinute() ,
    (uint8_t)Clock.getSecond() );
}

// this function allow us to name the file we are going to create by the day of the measure
void set_file_name(char *fileName,DateTime data){  
  
  char dataName[10];
  DateToString(data,dataName);
  
  fileName[0]  = dataName[0];
  fileName[1]  = dataName[1];
  fileName[2]  = dataName[2];
  fileName[3]  = dataName[3];
  fileName[4]  = dataName[5];
  fileName[5]  = dataName[6];
  fileName[6]  = dataName[8];
  fileName[7]  = dataName[9];
  
}

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}


void setup() {
  

  Wire.begin();
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  initSDcard();
  #ifdef SET_TIME
  setDS3231time();
  #endif
  dht.begin();
  
  
  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit"));
  }
 
    
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  

  ble.echo(false);
  eddyBeacon.begin(true);
  eddyBeacon.setURL("http://test1");
  eddyBeacon.startBroadcast();
  
}

void loop() {
  
  delay(3000);
  
  data.temperature = dht.readTemperature(); 
  data.humidity = dht.readHumidity();
  data.current_date = readDS3231date();
  
  set_file_name(fileName,data.current_date);
  #ifdef DEBUG
  Serial.println(fileName);
  #endif
  
  dataFile = SD.open(fileName, FILE_WRITE);

  if (dataFile) {
  char datalog[52];
  create_data_log_string(data,datalog);
  #ifdef DEBUG
  Serial.println(datalog);
  #endif
  dataFile.println(datalog);
  dataFile.close();
  
    
  }
  #ifdef DEBUG
  // if the file isn't open, pop up an error:
  else {
    Serial.println(F("error opening selected file"));
  }

  if (SD.exists(fileName)) {
    Serial.println(F("file searched exists."));
  } else {
    Serial.println(F("file searched doesn't exist."));
  }
  #endif
   eddyBeacon.setURL("http://test2");
  
}

ISR(TIMER1_COMPB_vect){

    
}
