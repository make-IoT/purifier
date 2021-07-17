/* This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details. */
#include <SparkFun_SCD30_Arduino_Library.h>
#include <Wire.h>
#include <bsec.h>
#include <Ticker.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>

int CO2 = 0 ;
//Reading CO2, humidity and temperature from the SCD30 By: Nathan Seidle SparkFun Electronics 

//https://github.com/sparkfun/SparkFun_SCD30_Arduino_Library

SCD30 airSensorSCD30; // Objekt SDC30 Umweltsensor
int IAQ = 0 ;
/* 
 Bosch BSEC Lib, https://github.com/BoschSensortec/BSEC-Arduino-library
 The BSEC software is only available for download or use after accepting the software license agreement.
 By using this library, you have agreed to the terms of the license agreement: 
 https://ae-bst.resource.bosch.com/media/_tech/media/bsec/2017-07-17_ClickThrough_License_Terms_Environmentalib_SW_CLEAN.pdf */
Bsec iaqSensor;     // Create an object of the class Bsec 
Ticker Bsec_Ticker; // schedule cyclic update via Ticker 

// ------------------------   Helper functions Bosch Bsec - Lib 
void checkIaqSensorStatus(void)
{ 
  String output; 
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      for (;;) {
        Serial.println(output);
        delay(500);
      } // Halt in case of failure 
    } 
    else {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      for (;;){
        Serial.println(output);
        delay(500);
      }  // Halt in case of failure 
    } 
    else {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    }
  }
}

// Housekeeping: scheduled update using ticker-lib
void iaqSensor_Housekeeping(){  // get new data 
  iaqSensor.run();
}

// Adafruit Feather 7 Segment https://learn.adafruit.com/adafruit-7-segment-led-featherwings/overview
Adafruit_7segment matrix7Seg = Adafruit_7segment(); // 7Segment Feather
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2,13,NEO_GRBW + NEO_KHZ800);


void setup(){ // Einmalige Initialisierung
  WiFi.forceSleepBegin(); // Wifi off
  Wire.begin(); // ---- Initialisiere den I2C-Bus 

  if (Wire.status() != I2C_OK) Serial.println("Something wrong with I2C");

  if (airSensorSCD30.begin() == false) {
    Serial.println("The SCD30 did not respond. Please check wiring."); 
    while(1) {
      yield(); 
      delay(1);
    } 
  }

  airSensorSCD30.setAutoSelfCalibration(false); // Sensirion no auto calibration

  airSensorSCD30.setMeasurementInterval(2);     // CO2-Messung alle 5 s

  Serial.begin(115200);
  iaqSensor.begin(BME680_I2C_ADDR_PRIMARY, Wire);
  String output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(output);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();  
  iaqSensor_Housekeeping();
  Bsec_Ticker.attach_ms(3000, iaqSensor_Housekeeping);

  matrix7Seg.begin(0x70); // ---- Initialisiere 7Segment Matrix
  matrix7Seg.clear();     // 
  matrix7Seg.writeDisplay();

  pixels.begin();//-------------- Initialisierung Neopixel
  delay(1);
  pixels.show();
  pixels.setPixelColor(0,0,0,0,0); // alle aus
  pixels.setPixelColor(1,0,0,0,0);
  pixels.show();                 // und anzeigen

  Serial.println();
  analogWriteFreq(0);

  analogWrite(15 , 100);

  Wire.setClock(100000L);            // 100 kHz SCD30 
  Wire.setClockStretchLimit(200000L);// CO2-SCD30
}

void loop() { // Kontinuierliche Wiederholung 
  CO2 = airSensorSCD30.getCO2() ;
  IAQ = iaqSensor.iaq ;
  matrix7Seg.print(CO2);
  matrix7Seg.writeDisplay();
  TestCO2();
  TestIAQ();
  delay( 5000 );
}

void TestCO2()
{
  if (( ( CO2 ) < ( 800 ) ))
  {
    pixels.setPixelColor(1,0,30,0,0);
    pixels.show();
    Serial.print("Alles ok");
    Serial.println();
  }
  else
  {
    if (( ( CO2 ) < ( 1000 ) ))
    {
      pixels.setPixelColor(1,30,30,0,0);
      pixels.show();
      Serial.print("Bitte: Querlueften!");
      Serial.println();
    }
    else
    {
      pixels.setPixelColor(1,40,0,0,0);
      pixels.show();
      Serial.print("Alarm: Querlueften!");
      Serial.println();
    }
  }
  delay( 5000 );
}

void TestIAQ()
{
  if (( ( iaqSensor.iaqAccuracy ) >= ( 1 ) ))
  {
    if (( ( IAQ ) <= ( 100 ) ))
    {
      analogWriteFreq(100);
      analogWrite(15 , 100);
      pixels.setPixelColor(0,0,30,0,0);
      pixels.show();
      Serial.print("Stufe1");
      Serial.println();
    }
    else
    {
      if (( ( IAQ ) <= ( 150 ) ))
      {
        analogWriteFreq(220);
        analogWrite(15 , 100);
        pixels.setPixelColor(0,30,30,0,0);
        pixels.show();
        Serial.print("Stufe2");
        Serial.println();
      }
      else
      {
        analogWriteFreq(300);
        analogWrite(15 , 100);
        pixels.setPixelColor(0,40,0,0,0);
        pixels.show();
        Serial.print("Stufe3");
        Serial.println();
        pixels.setPixelColor(1,40,0,0,0);
        pixels.show();
        Serial.print("Alarm: Querlueften!");
        Serial.println();
      }
    }
  }
  else
  {
    pixels.setPixelColor(0,0,0,30,0);
    pixels.show();
    Serial.print("IAQ not ready!");
    Serial.println();
  }
}
