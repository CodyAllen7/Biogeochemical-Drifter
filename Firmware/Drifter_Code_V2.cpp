/* 
 * Project Biogeochemical Drifter
 * Author: Stephen Lail
 * Date: 6/20/24
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
void setup();
void loop();
void printToFile();
void serialPrintGPSTime();
void serialPrintGPSLoc();
void step1();
void step2();
void step3();
void step4();
void receive_reading(Ezo_board & Sensor);

long real_time;
int millis_now;

//--------SD configuration------------

#include <SdFat.h>
const int SD_CHIP_SELECT = D5;
SdFat SD;

char filename[] = "YYMMDD00.csv"; // filename in year, month, day, 00-99 file number
bool filenameCreated = false;

//-------GPS---------------------------------

#include <Adafruit_GPS.h>
#define GPSSerial Serial1
Adafruit_GPS GPS( &GPSSerial);
uint32_t timer = millis();

//------pH Sensor----------------------------

#include <Ezo_i2c.h>
#include <Wire.h>
#include <Ezo_i2c_util.h>
#include <iot_cmd.h>
#include <sequencer4.h>
void step1();
void step2();
void step3();
void step4();
void receive_reading(Ezo_board & Sensor);

Ezo_board ph = Ezo_board(99, "PH");
Ezo_board rtd = Ezo_board(102, "TEMP");
Ezo_board DO = Ezo_board(97, "DO");
Ezo_board ec = Ezo_board(100, "EC");

void step1(); //Read RTD circuit
void step2(); //temperature compensation
void step3(); //send a read command
void step4(); // print data to serial monitor and to SD card

Sequencer4 Seq(&step1, 1000, &step2, 1000, &step3, 1000, &step4, 1000);

//------LED Light-----------------

const pin_t MY_LED = D7;
bool led_state = HIGH;

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

void setup(){
    pinMode(MY_LED, OUTPUT);
    Wire.begin();

    Serial.begin(115200);
    Serial.println("Adafruit GPS Sensor Test");

    GPS.begin(9600);

    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

    if (!SD.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)){
        Serial.println("failed to open card");
        return;
    }
}

void loop(){

    Seq.run();
    GPS.read();


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

            Serial.println("Reached latitude and longitude print statements");

            Serial.print("Latitude: ");
            Serial.print(GPS.latitude, 4); // Print latitude with 4 decimal places
            Serial.print(" Longitude: ");
            Serial.println(GPS.longitude, 4);

        }
        printToFile();

    }
    
    

}

// ---------------Steps for EZO sensors------------

void step1(){
    rtd.send_read_cmd(); //read the RTD circuit

}

void step2(){
    if ((rtd.get_error() == Ezo_board::SUCCESS) && (rtd.get_last_received_reading() > -1000.0)){ //If temperature reading has been received and is valid
     ph.send_cmd_with_num("T,", rtd.get_last_received_reading());
     ec.send_cmd_with_num("T,", rtd.get_last_received_reading());
     DO.send_cmd_with_num("T,", rtd.get_last_received_reading());
    }
    else{
        ph.send_cmd_with_num("T,", 25.0);
        ec.send_cmd_with_num("T,", 25.0);
        DO.send_cmd_with_num("T,", 25.0);
    }
}

void step3(){
    ph.send_read_cmd();
    ec.send_read_cmd();
    DO.send_read_cmd();
}

void step4(){
    receive_and_print_reading(rtd);
      if (rtd.get_error() == Ezo_board::SUCCESS);
      Serial.print("  ");
    Serial.println();

    receive_and_print_reading(ph);
      if (ph.get_error() == Ezo_board::SUCCESS);
      Serial.print("  ");
    Serial.println();

    receive_and_print_reading(ec);
      if (ec.get_error() == Ezo_board::SUCCESS);
      Serial.print("  ");
    Serial.println();

    receive_and_print_reading(DO);
      if (DO.get_error() == Ezo_board::SUCCESS);
      Serial.print("  ");
    Serial.println();

      
}

// ---------------Print to SD Card--------------
void printToFile(){

    if (!filenameCreated){
        //Year, month, day for filename
        int filenum = 0; //start at zero and increment by one if file exists
        sprintf(filename, "%02d%02d%02d%02d.csv", GPS.year, GPS.month, GPS.day, filenum);

        while (SD.exists(filename)) {
            filenum++;
            sprintf(filename, "%02d%02d%02d%02d.csv", GPS.year, GPS.month, GPS.day, filenum);

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
        dataFile.print(GPS.milliseconds);
        dataFile.print(",");

        //Elapsed time
        dataFile.print(millis()/1000);
        dataFile.print(",");

        //Location
        dataFile.print(GPS.latitude, 5);
        dataFile.print(",");
        dataFile.print(GPS.lat);
        dataFile.print(",");
        dataFile.print(GPS.longitude, 5);
        dataFile.print(",");
        dataFile.print(GPS.lon);
        dataFile.print(",");

        //Altitude
        dataFile.print(GPS.altitude);
        dataFile.print(",");
        dataFile.print(GPS.speed);
        dataFile.print(",");

        //Angle
        dataFile.print(GPS.angle);
        dataFile.print(",")
        

       


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