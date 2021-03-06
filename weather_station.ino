String v = "0.21"; 
/* Weather station v0.21 by David Réchatin  
 
 Weather station with web connexion
 
 https://github.com/DavidRechatin/weather-station
 
 This software is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 The circuit:
 Arduino MEGA 2650 R3
 
 Sensor : TEMT6000
 * VCC connection of the sensor attached to +5V
 * GND connection of the sensor attached to ground
 * SIG connection of the sensor attached to analog pin A8
 
 Sensor : RHT03 (alias DHT22)
 * Pin1 connection of the sensor attached to +5V
 * Pin2 connection of the sensor attached to digital pin 30 with 1K resistor to +5V
 * Pin3 connection of the sensor attached to ground
 * Pin4 connection of the sensor attached to ground
 
 Sensor : SHT15
 * VCC  connection of the sensor attached to +5V
 * DATA connection of the sensor attached to digital pin 28
 * SCK  connection of the sensor attached to digital pin 29
 * GND  connection of the sensor attached to ground
 
 Sensor : BMP085
 * VNI  connection of the sensor attached to +5V
 * SLC connection of the sensor attached to digital pin 21 > I2C bus
 * SDA  connection of the sensor attached to digital pin 20 > I2C bus
 * GND  connection of the sensor attached to ground
 
 Sensor : wind speed
 * Pin2 connection of the sensor attached to +5V
 * Pin3 connection of the sensor attached to digital pin 2 > interrupt 0 with 10K resistor to ground
 
 Sensor : wind direction
 * Pin1 connection of the sensor attached to analog pin A9 with 10K resistor to +5V
 * Pin4 connection of the sensor attached to ground
 
 Sensor : rain gauge
 * Pin2 connection of the sensor attached to +5V
 * Pin3 connection of the sensor attached to digital pin 3 > interrupt 1 with 10K resistor to ground
 
 LED working in progress in loop
 * to digital pin 40
 
 Button
 * Pin1 connection of the button attached to +5V
 * Pin2 connection of the button to digital pin 18 > interrupt 5 with 10K resistor to ground
 
 LCD display 20x4 (Hitachi 44780 compatible - I2C bus)
 * 5V  connection of the sensor attached to +5V
 * S connection of the sensor attached to digital pin 21 > I2C bus
 * D  connection of the sensor attached to digital pin 20 > I2C bus
 * GND  connection of the sensor attached to ground
 
 http://www.rechatin.com/
 
 created 31 oct 2012
 by David Réchatin  
 
 v0.1 31/10/2012  : test RHT03 sensor (temperature + humdity) and TEMT6000 sensor (luminosity)
 v0.2 01/11/2012  : add LCD display 2x16   
 v0.3 01/11/2012  : add SHT15 sensor (temperature + humdity)  
 v0.4 01/11/2012  : add BMP085 sensor (temperature + pressure)
 v0.5 01/11/2012  : add button for select display information 
 v0.6 02/11/2012  : add windvane 
 + begin wind speed with irq_windSpeed() and led wind sensor interrupt
 + begin rain gauge with irq_raingauge() and led rain gauge sensor interrupt 
 v0.7 06/11/2012  : add altitude of BMP085 sensor 
 v0.8 15/11/2012  : wind speed calculation with number of closure in each loop
 v0.9 15/11/2012  : wind speed advanced calculation 
 v0.10 15/11/2012 : rain height calculation 
 v0.11 01/12/2012 : fix negative temperature of RHT03 (with update library DHT22.ccp v0.5) 
 v0.12 09/12/2012 : BMP085 sensor : calculating pressure at sea level
 v0.13 10/12/2012 : define the pin numbers with const
 v0.14 16/12/2012 : add deported LCD display 4x20 (i2c bus)
 v0.15 02/02/2013 : remove LCD display 2x16
 v0.16 03/02/2013 : change windVane data storage
 v0.17 03/02/2013 : rewriting function : generate and send sata to LCD display
 v0.18 03/02/2013 : adjust RHT03 temperature value
 v0.19 03/02/2013 : add 'Data logger SD card' shield with DS1307 clock > active DS1307
 v0.20 03/02/2013 : various optimizations
 v0.21 20/05/2013 : add support of SD Card 
 + sort windvaneArrayDeg[16]
 + windSpeed bug fixe (create buffer with 13 caractere for sd card filename
 
 todo list :
 - SD card data logger
 - ethernet push data FTP
 
 */

// -------------------------------------
// Include the library code:
#include <Wire.h>
#include <CLCD.h>             // require by LCD display 4x20 bus I2C
#include <DHT22.h>            // require by RHT03
#include <SHT1x.h>            // require by SHT15
#include <Adafruit_BMP085.h>  // require by BMP085
#include <SPI.h>              // require by ethernet shield
#include <Ethernet.h>         // require by ethernet shield
#include <RTClib.h>           // require by DS1307 on Data logger SD card shield
#include <SD.h>               // require by Sd Card on Data logger SD card shield

// -------------------------------------
// Define PIN constant
#define TEMT6000_SIG_PIN 8   // TEMT6000 sensor > analog
#define RHT03_DATA_PIN 30    // RHT03 sensor
#define SHT15_DATA_PIN  28   // SHT15 sensor
#define SHT15_SCK_PIN 29     // SHT15 sensor
#define WINDVANE_PIN 9       // Wind vane sensor > analog
#define SD_CARD_CS 53       // SD Card CS Pin  (use 10 for Arduino Uno)
#define LED_PIN 40           // LED working in progress in loop

// Define IRQ constant
#define windSpeed_IRQ_NUMBER 0    // PIN 2
#define RAINGAUGE_IRQ_NUMBER 1    // PIN 3
#define BUTTON_IRQ_NUMBER 5       // PIN 18

// Define other constant
#define NUMBER_DISP_STATE 4    // number of menu on LCD display
#define WAIT_LOOP 5000         // time (in milli second) of wait in loop cycle

// -------------------------------------
// Variable declaration
String debug = "" ;       // for debuging
int disp_state = 0;       // use by interrup 5 : function irq_button()
char buffer8[8];          // buffer with 8 caracteres
char buffer13[13];        // buffer with 13 caracteres 
char* tmp_char = " " ;    // temporary variable for increase code lisibilty
volatile int countwindSpeed = 0 ;            // count wind speed contact
unsigned long firstContactTime = 0 ;         // first time of wind speed interupt
volatile unsigned long lastContactTime = 0 ; // last time of wind speed interupt
volatile int countRain = 0 ;                 // count rain gauge contact
DateTime now ;            // current date and time storage

// Measure variables declaration
float TEMT6000_l = 0 ;   // luminosity
float RTH03_t = 0 ;      // temperature
float RTH03_t_adjust = 0.7 ;      // temperature adjust
float RTH03_h = 0 ;      // humdity
float SHT15_t = 0 ;      // temperature
float SHT15_h = 0 ;      // humdity
float BMP085_t = 0 ;     // temperature
float BMP085_p = 0 ;     // pressure
float windVaneDeg = 0 ;  // wind direction degré 
char* windVaneStr = " " ;  // wind direction string 
float windSpeed = 0 ;    // wind speed
float rainHeight = 0 ;   // height of rain
float t = 0 ;            // temperature
float h = 0 ;            // humdity

// -------------------------------------
// INITIALIZE SENSORS, LCD AND SHIELDS

// initialize clock : DS1307
RTC_DS1307 RTC;
// initialize sensor : RHT03 (alias DHT22)
DHT22 myDHT22(RHT03_DATA_PIN);
// initialize LCD display 4x20 
CLCD lcd_i2c(0x06,20,4);
// initialize sensor : SHT15
SHT1x sht1x(SHT15_DATA_PIN, SHT15_SCK_PIN);
// initialize sensor : BMP085
Adafruit_BMP085 bmp;
// Initialize ethernet shield
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x0D, 0x80, 0xF3 };  // MAC address printed on a sticker on the shield
IPAddress server(192,168,0,230);   // IP address of server
EthernetClient client;   // Initialize the Ethernet client library

// ###############################################################################
// BEGIN FUNCTIONS


// =============================================================
// CALCULATION WIND DIRECTION

void getWindVane() {
  float windvaneArrayVal[16] = {
    73.5, 87.5, 108.5, 155, 215, 266.5, 348, 436, 533, 617.5, 668.5, 746, 809, 860, 918.5, 1023                                              }; // median values measured at the analog input  
  char *windvaneArrayStr[16] = {
    "ONO","OSO", "O","NNO", "NO", "NNE", "N", "SSO", "SO", "ENE", "NE", "SSE", "S", "ESE", "SE", "E"                                              };  // wind directions
  float windvaneArrayDeg[16] = {
    67.5, 112.5, 90, 22.5, 45, 337.5, 0, 157.5, 135, 292.5, 315, 202.5, 180, 247.5, 225, 270  }; // degre direction  
  unsigned int val; // analog input value
  byte x;  // index

    val = analogRead(WINDVANE_PIN);  // read analog input pin 9
  for (x=0; x<16; x++) {   // search index
    if (windvaneArrayVal[x] >= val)  // in windvaneVal with a value below the median value higher
      break;
  }
  // return the wind direction using index
  windVaneDeg = windvaneArrayDeg[x];  // wind degre direction
  //windVaneStr = String(windvaneArrayStr[x]);  // wind string direction 
  strcpy(windVaneStr,windvaneArrayStr[x]);
}

// =============================================================
// CALCULATION WIND SPEED
float getwindSpeed() { 
  float calc = 0 ;
  if (countwindSpeed == 0) // if we haven't contact
  {  
    calc = 0; // no wind
  } 
  else   // we have contact (one or more)
  { 
    // calculation of speed 
    calc = countwindSpeed / (float(lastContactTime - firstContactTime) / 1000) * 1.2 ; // impulses / time * 2,4 km/h per second / 2 switch closure per revolution
    firstContactTime = lastContactTime;  // stores the time of last contact
    countwindSpeed = 0;
  }  
  return float(calc); 
}

// =============================================================
// CALCULATION RAIN GAUGE
float getRainHeight() {    
  float calc = 0 ;  
  calc = countRain * 0.2794 ; 
  //countRain = 0;
  return float(calc);
}

// =============================================================
// DISPLAY DATA
void display_lcd() {   
  lcd_i2c.clear();
  // -------------------------------------
  // Generate and send data to lcd display
  switch (disp_state) { // state of information display
  case 0: // defaut display
    // line 1  
    lcd_i2c.setCursor(0, 0);  // (row,col)
    lcd_i2c.print("T ");    
    t = (RTH03_t + SHT15_t + BMP085_t) / 3;  
    lcd_i2c.print(t); 
    lcd_i2c.print("*C ");  
    lcd_i2c.setCursor(0, 12);  // (row,col)    
    lcd_i2c.print(now.hour()); 
    lcd_i2c.print(":"); 
    lcd_i2c.print(now.minute()); 
    lcd_i2c.print(":");  
    lcd_i2c.print(now.second()); 
    // line 2 
    lcd_i2c.setCursor(1, 0);  // (row,col)
    lcd_i2c.print("P ");    
    dtostrf(BMP085_p,1,1,buffer8);  // conv. float to char*
    lcd_i2c.print(buffer8); 
    lcd_i2c.print("mBar L ");    
    dtostrf(TEMT6000_l,1,1,buffer8);  // conv. float to char*
    lcd_i2c.print(buffer8); 
    lcd_i2c.print("%");   
    // line 3 
    lcd_i2c.setCursor(2, 0);  // (row,col)
    lcd_i2c.print("V ");    
    dtostrf(windSpeed,1,1,buffer8);  // conv. float to char*  
    lcd_i2c.print(buffer8); 
    lcd_i2c.print("km/h > ");    
    lcd_i2c.print(windVaneStr); 
    // line 4 
    lcd_i2c.setCursor(3, 0);  // (row,col)
    lcd_i2c.print("H ");    
    h = (RTH03_h + SHT15_h) / 2;
    dtostrf(h,1,1,buffer8);  // conv. float to char*  
    lcd_i2c.print(buffer8); 
    lcd_i2c.print("% Pl ");    
    lcd_i2c.print(rainHeight); 
    lcd_i2c.print("mm"); 
    break;
  case 1: // Temperature detail  
    // line 1  
    lcd_i2c.setCursor(0, 0);  // (row,col)
    lcd_i2c.print("Temperature *C");      
    // line 2
    lcd_i2c.setCursor(1, 0);  // (row,col)
    lcd_i2c.print("RHT03  ");  
    lcd_i2c.print(RTH03_t);    
    // line 3
    lcd_i2c.setCursor(2, 0);  // (row,col)
    lcd_i2c.print("SHT15  ");  
    lcd_i2c.print(SHT15_t);    
    // line 4
    lcd_i2c.setCursor(3, 0);  // (row,col)
    lcd_i2c.print("BMP085 ");  
    lcd_i2c.print(BMP085_t); 
    break;
  case 2: // Humidity detail  
    // line 1  
    lcd_i2c.setCursor(0, 0);  // (row,col)
    lcd_i2c.print("Humidite %");      
    // line 2
    lcd_i2c.setCursor(1, 0);  // (row,col)
    lcd_i2c.print("RTH03 ");  
    lcd_i2c.print(RTH03_h);    
    // line 3
    lcd_i2c.setCursor(2, 0);  // (row,col)
    lcd_i2c.print("SHT15 ");  
    lcd_i2c.print(SHT15_h);    
    break;
  case 3: // information
    lcd_i2c.setCursor(0, 0);  // (row,col)
    lcd_i2c.print("Station meteo");    
    lcd_i2c.setCursor(1, 0);  // (row,col)
    lcd_i2c.print("Version " + v);   
    lcd_i2c.setCursor(3,0);
    lcd_i2c.print(now.day(), DEC);
    lcd_i2c.print('/');
    lcd_i2c.print(now.month(), DEC);
    lcd_i2c.print('/');
    lcd_i2c.print(now.year(), DEC);
    lcd_i2c.print(' ');
    lcd_i2c.print(now.hour(), DEC);
    lcd_i2c.print(':');
    lcd_i2c.print(now.minute(), DEC);
    lcd_i2c.print(':');
    lcd_i2c.print(now.second(), DEC);
    break;
  default: 
    disp_state = 0; // security...
  }
  lcd_i2c.cursor_off();
}

// =============================================================
// BUTTON INTERUPTION
void irq_button() // interrupt 5
{
  //  Serial.println("Button interuption !");
  disp_state++;   // next state of display
  if (disp_state == NUMBER_DISP_STATE) {  // state no exist
    disp_state=0;  // loop to state 0
  }
}

// =============================================================
// WIND SPEED SENSOR INTERUPTION
void irq_windSpeed() // interrupt 0
{
  countwindSpeed++; 
  lastContactTime = millis();
}

// =============================================================
// RAIN GAUDE SENSOR INTERUPTION
void irq_raingauge() // interrupt 1
{   
  countRain++; 
}

// END FUNCTIONS
// ###############################################################################


// ###############################################################################
// BEGIN SETUP
void setup()
{
  Serial.begin(9600);
  Serial.println("Setup begin");
  pinMode(40, OUTPUT); // define pin of LED reading sensors

  // interruption by button
  attachInterrupt (BUTTON_IRQ_NUMBER, irq_button, RISING);
  // interruption by wind wensor
  attachInterrupt (windSpeed_IRQ_NUMBER, irq_windSpeed, FALLING);
  // interruption by rain gauge
  attachInterrupt (RAINGAUGE_IRQ_NUMBER, irq_raingauge, FALLING);

  // DS1307 clock on Data logger SD card shield
  RTC.begin();
  /*
  if (! RTC.isrunning()) {
   Serial.println("RTC is NOT running!");
   }
   */

  // SD Card on Data logger SD card shield
  //Serial.print("Initializing SD card...");
  pinMode(SD_CARD_CS, OUTPUT);
  if (!SD.begin(SD_CARD_CS)) {
    //Serial.println("Card failed, or not present");
    return;
  }
  //Serial.println("card initialized.");  

  // LCD Display
  lcd_i2c.init(); 
  lcd_i2c.backlight();

  // BMP085
  if (!bmp.begin()) {
    //    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {
    }
  }

  /*
  // Ethernet shield
   Serial.println("Begin Ethernet shield...");
   // start the Ethernet connection:
   if (Ethernet.begin(mac) == 0) {
   Serial.println("Failed to configure Ethernet using DHCP");
   // no point in carrying on, so do nothing forevermore:
   for(;;)
   ;
   }
   */

  Serial.println("Setup end");
}
// END SETUP
// ###############################################################################




// ###############################################################################
// BEGIN LOOP
void loop()
{  
  Serial.println("Loop...");
  digitalWrite(LED_PIN, HIGH);   // turn ON the LED working in progress in loop

  // initialize variable
  debug = "";

  // read date and time
  now = RTC.now();
  
// #######################################
// A faire : stocker les infos de now dans des variables sur 2 caractères

  // =============================================================
  // READ SENSORS DATA

  // read data > TEMT6000
  TEMT6000_l = float(analogRead(TEMT6000_SIG_PIN))*100/1023;       // read analog input pin 8

  // read data > RHT03 (alias DHT22)
  DHT22_ERROR_t errorCode; 
  errorCode = myDHT22.readData(); 
  switch(errorCode)
  {
  case DHT_ERROR_NONE:
    RTH03_t = RTH03_t_adjust + myDHT22.getTemperatureC();
    RTH03_h = RTH03_t_adjust + myDHT22.getHumidity();
    break;
  case DHT_ERROR_CHECKSUM:
    debug += "check sum error ";
    RTH03_t = myDHT22.getTemperatureC();
    RTH03_h = myDHT22.getHumidity();
    break;
  case DHT_BUS_HUNG:
    debug += "BUS Hung ";
    break;
  case DHT_ERROR_NOT_PRESENT:
    debug += "Not Present ";
    break;
  case DHT_ERROR_ACK_TOO_LONG:
    debug += "ACK time out ";
    break;
  case DHT_ERROR_SYNC_TIMEOUT:
    debug += "Sync Timeout ";
    break;
  case DHT_ERROR_DATA_TIMEOUT:
    debug += "Data Timeout ";
    break;
  case DHT_ERROR_TOOQUICK:
    debug += "Polled to quick ";
    break;
  }

  // read data  > STH15
  SHT15_t = sht1x.readTemperatureC(); // it's slow !!
  SHT15_h = sht1x.readHumidity(); // it's slow !!

  // read data  > BMP085
  BMP085_t = bmp.readTemperature();
  // If altitude = 490m, then coefficient = 0.94326373664205762142   | pressure at sea level = pressure /((1-(altitude/44330))^5.255)
  BMP085_p = bmp.readPressure()/0.94326373664205762142; // pressure at sea level in Pa
  BMP085_p /= 100;   // pressure in mBar (1 mBar = 100 Pa)

  // read data > wind direction
  getWindVane(); 

  // read data > wind speed
  windSpeed = getwindSpeed(); 

  // read data > rain height
  rainHeight = getRainHeight(); 

  // generate and send data to display
  display_lcd();

// #######################################
// Work on progress

  // Write data on SD Card
  String dataString = "{\"date\":\"";
  dataString += now.day();
  dataString += "/";
  dataString += now.month();
  dataString += "/";
  dataString += now.year();
  dataString += "\",\"time\":\"";
  dataString += now.hour();
  dataString += ":";
  dataString += now.minute();
  dataString += ":";
  dataString += now.second();  
  dataString += "\",\"TEMT6000_l\":\"";
  dtostrf(TEMT6000_l,1,1,buffer8);  // conv. float to char*
  dataString +=  buffer8;  
  dataString += "\",\"RTH03_t\":\"";
  dtostrf(RTH03_t,1,1,buffer8);  // conv. float to char*
  dataString +=  buffer8;
  dataString += "\",\"RTH03_h\":\"";
  dtostrf(RTH03_h,1,1,buffer8);  // conv. float to char*
  dataString +=  buffer8; 
  dataString += "\",\"SHT15_t\":\"";
  dtostrf(SHT15_t,1,1,buffer8);  // conv. float to char*
  dataString +=  buffer8;
  dataString += "\",\"SHT15_h\":\"";
  dtostrf(SHT15_h,1,1,buffer8);  // conv. float to char*
  dataString +=  buffer8; 
  dataString += "\",\"BMP085_t\":\"";
  dtostrf(BMP085_t,1,1,buffer8);  // conv. float to char*
  dataString +=  buffer8;  
  dataString += "\",\"BMP085_p\":\"";
  dtostrf(BMP085_p,1,1,buffer8);  // conv. float to char*
  dataString +=  buffer8;
  //dataString += "\",\"windVaneDeg\":\"";
  //dtostrf(windVaneDeg,1,1,buffer8);  // conv. float to char*
  //dataString +=  buffer8;
  dataString += "\",\"windSpeed\":\"";
  dtostrf(windSpeed,1,1,buffer8);  // conv. float to char*
  dataString +=  buffer8;
  dataString += "\",\"rainHeight\":\"";
  dtostrf(rainHeight,1,1,buffer8);  // conv. float to char*
  dataString +=  buffer8;
  dataString += "\"";
  dataString += "}";

  String fileName = "";
  fileName += now.year();
  fileName += now.month();
  fileName += now.day();
  fileName += ".txt";  
  fileName.toCharArray(buffer13, 13); 

  File dataFile = SD.open(buffer13, FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  }  
  else {
    Serial.println("error opening ");
    Serial.println(fileName);
  } 

  // =============================================================
  // OTHER TREATMENTS
  digitalWrite(LED_PIN, LOW);   // turn OFF the LED working in progress in loop
  delay(WAIT_LOOP);   // wait
}

// END LOOP
// ###############################################################################



//
// END OF FILE
//
























