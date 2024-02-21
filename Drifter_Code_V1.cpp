#include "Particle.h"
//Keeping Blue blinking light as indicator 
//Pin D7 is defined as the blue light 

//initial code to get GPS to work


void setup();
void loop();
void printToFile();
void serialPrintGPSTime();
void serialPrintGPSLoc();

long real_time;
int millis_now;

//--------SD configuration------------

#include "SdFat.h"
const int SD_CHIP_SELECT = D5;
SdFat SD;

char filename[] = "YYMMDD00.csv"; // filename in year, month, day, 00-99 file number
bool filenameCreated = false;

//-------GPS---------------------------------

#include <Adafruit_GPS.h>
#define GPSSerial Serial1
Adafruit_GPS GPS( &GPSSerial);
uint32_t timer = millis();

//------LED Light-----------------

const pin_t MY_LED = D7;
bool led_state = HIGH;

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

void setup(){
    pinMode(MY_LED, OUTPUT);

    Serial.begin(115200);
    Serial.println("Adafruit GPS Sensor Test");

    GPS.begin(9600);

    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

    if (!SD.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)){
        Serial.println("failed to open car");
        return;
    }
}

void loop(){

    char c = GPS.read();  //This line is having issues because it isnt used anywhere else

    if (GPS.newNMEAreceived()){
        if (!GPS.parse(GPS.lastNMEA()))
        return;
    }
    if (millis() - timer > 1000){
        timer = millis();
        serialPrintGPSTime();

        if (GPS.fix){
            led_state = !led_state;
            digitalWrite(MY_LED, led_state);
            serialPrintGPSLoc();

        }
        printToFile();

    }
}
// ---------------Print to SD Card--------------
void printToFile(){

    if (!filenameCreated){
        //Year, month, day for filename
        int filenum = 0; //start at zero and increment by one if file exists
        sprintf(filename, "%02d%02d%02d%02d.csv", GPS.year, GPS.month, GPS.day, filenum); //This line is having issues mentioned below
      
	//"resource": "/c:/Users/Stephen/.particle/Boron_Blink/src/Drifter_Code_V1.cpp",
	//"owner": "cpptools",
	//"severity": 4,
//	"message": "'.csv' directive writing 4 bytes into a region of size between 2 and 5 [-Wformat-overflow=]",
//	"source": "gcc",
//	"startLineNumber": 87,
//	"startColumn": 44,
//	"endLineNumber": 87,
//	"endColumn": 44


        while (SD.exists(filename)) {
            filenum++;
            sprintf(filename, "%02d%02d%02d%02d.csv", GPS.year, GPS.month, GPS.day, filenum); // Similar issue mentioned earlier

        }
        filenameCreated = true;
    }
    Serial.println(filename);

    File dataFile = SD.open(filename, FILE_WRITE);

    if(dataFile){
       //Date
        dataFile.print(GPS.month, DEC);
        dataFile.print('/');
        dataFile.print(GPS.day, DEC);
        dataFile.print("/20");
        dataFile.print(GPS.year, DEC);
        dataFile.print(",");
        //Time
        dataFile.print(GPS.hour, DEC);
        dataFile.print(':');
        if (GPS.minute < 10){
            dataFile.print('0');
        }
        dataFile.print(GPS.minute, DEC);
        dataFile.print(':');
        if(GPS.seconds <10){
            dataFile.print('0');
        }
        dataFile.print(GPS.seconds, DEC);
        dataFile.print(".");
        if(GPS.milliseconds < 10){
            dataFile.print("00");
        } else if (GPS.milliseconds >9 && GPS.milliseconds <100){
            dataFile.print("0");
        }
        dataFile.println(GPS.milliseconds);
        dataFile.print(",");

        //Elapsed time
        dataFile.print(millis()/1000);
        dataFile.print(",");

        //Location
        dataFile.print(GPS.latitude, 4);
        dataFile.print(",");
        dataFile.print(GPS.lat);
        dataFile.print(",");
        dataFile.print(GPS.longitude, 4);
        dataFile.print(",");
        dataFile.print(GPS.lon);
        dataFile.print(",");

        //Altitude
        dataFile.print(GPS.altitude);
        dataFile.print(",");
        dataFile.print(GPS.speed);
        dataFile.print(",");

        //Angle
        dataFile.println(GPS.angle);
        dataFile.close();


    }
    else{
        Serial.println("error opening datalog.txt");
    }
}
void serialPrintGPSTime(){
    Serial.print("\nTime");

    //Hour
    if (GPS.hour < 10){
        Serial.print('0');

    }
    Serial.print(GPS.hour, DEC);
    Serial.print(':');

    //Minute
    if (GPS.minute <10){
        Serial.print('0');
    }
    Serial.print(GPS.minute, DEC);
    Serial.print(':');

    //Seconds
    if (GPS.seconds < 10) {
    Serial.print('0');
  }
  Serial.print(GPS.seconds, DEC);
  Serial.print('.');
  if (GPS.milliseconds < 10) {
    Serial.print("00");
  } else if (GPS.milliseconds > 9 && GPS.milliseconds < 100) {
    Serial.print("0");
  }
  Serial.println(GPS.milliseconds);
  
  Serial.print("Date: ");
  Serial.print(GPS.month, DEC);
  Serial.print('/');
  Serial.print(GPS.day, DEC);
  Serial.print("/20");
  Serial.println(GPS.year, DEC);
  
  Serial.print("Fix: ");
  Serial.print((int) GPS.fix);
  
  Serial.print(" quality: ");
  Serial.println((int) GPS.fixquality);
}

void serialPrintGPSLoc() {
  Serial.print("Location: ");
  Serial.print(GPS.latitude, 4);
  Serial.print(GPS.lat);
  Serial.print(", ");
  Serial.print(GPS.longitude, 4);
  Serial.println(GPS.lon);
}

