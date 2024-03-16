#include <Arduino.h>

//file system bits
#include "FS.h"
#include <LittleFS.h>
#define FORMAT_LITTLEFS_IF_FAILED true

//watchdog timer bits
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */

#define TIME_TO_SLEEP  15 
/* 
Time ESP32 will go to sleep (in seconds) This sets the sample rate of the datalogger 
If using with a USB power bank you will want to keep this at 15 seconds and adjust the bootCountRepeat for your sample rate.
This tricks the power back into staying on by pulsing a decent power draw every 15 seconds. 

*/
RTC_DATA_ATTR int bootCount = 0;

int bootCountRepeats=4; //number of  sleep cycles before it takes a reading. 

int mydata=0;
int switchedPower=2;
int overWriteSwitch=19;
int overWriteSwitchState=1;
int readWriteSwitch=18;
int readWriteSwitchState=1;
int inputPin=34;         //adc input
int readTotal, reading; //some variables for the ADC
int numReadings=30; //for smoothing out the analog reading as the esp ADC is a bit noisy
void setup() {
  Serial.begin(115200);
  Serial.println("Setup ESP32 to sleep for " + String(TIME_TO_SLEEP) +
  " Seconds");
  pinMode (switchedPower, OUTPUT);
  pinMode (overWriteSwitch, INPUT_PULLUP);
  pinMode (readWriteSwitch, INPUT_PULLUP);
  ++bootCount; 
  Serial.println("Boot number: " + String(bootCount));

  
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
     {
      Serial.println("LittleFS Mount Failed");
      return;
     }
   
   else
   {
       Serial.println("Little FS Mounted Successfully");
   }

 
}

void loop() {
   readWriteSwitchState = digitalRead(readWriteSwitch);

  delay(100);
  
if (readWriteSwitchState==0) //we are in replay mode
{  
  Serial.println ("replay Mode");
  readFile(LittleFS, "/soiled.txt"); // Read the contents of the file
  delay(30000); //wait for 30 seconds so you can copy the data from the serial terminal. One day this could be a big webpage routine
}

if(readWriteSwitchState==1) //we are in read mode
{
     esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  if(bootCount<bootCountRepeats)
  {
 digitalWrite (switchedPower, HIGH);
 delay(200);
 digitalWrite (switchedPower, LOW);
 Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();             //go to sleep
  
  }
  
    bootCount=0;
  
  delay(100);
  Serial.println("read mode");
   digitalWrite (switchedPower, HIGH);
 delay(200);
 digitalWrite (switchedPower, LOW);
  digitalWrite (switchedPower, HIGH);
  delay(800); //let the power supply stablise
    
    for (int i=0; i<=numReadings;i++)
{
readTotal+=analogRead(inputPin);
delay(1);  
}
mydata=readTotal/numReadings;  

    Serial.println (mydata);

      // Check if the file already exists to prevent overwritting existing data
   bool fileexists = LittleFS.exists("/soiled.txt");
   //Serial.print(fileexists);
   if(!fileexists) {
       Serial.println("File doesn’t exist");
       Serial.println("Creating file...");
       // Create File and add header
       writeFile(LittleFS, "/soiled.txt", "soiled \r\n");
   }
   //Check whether overwrite switch is closed, in which case overwrite

   overWriteSwitchState = digitalRead(overWriteSwitch);
   if (overWriteSwitchState==LOW)
   { 
    Serial.println("Overwrite switch is closed");
    Serial.println("Creating file...");
       // Create File and add header
       writeFile(LittleFS, "/soiled.txt", "soiled \r\n");
   }
   else {
       Serial.println("File already exists");
   }
    appendFile(LittleFS, "/soiled.txt", (String(mydata)+ "\r\n").c_str()); //Append data to the file
    delay (500); //wait a second for stability
      
    digitalWrite (switchedPower, LOW); 
    
    
    

    //zzzzzzz

    
    delay(1000); //have a bit of a lie in
  
}
   
}



void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}
