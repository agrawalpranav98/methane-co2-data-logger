/*
 * Project: Environmental Gas Monitoring Data Logger
 * Author: Pranav Agrawal
 *
 * Description:
 * Standalone embedded data logger for methane, CO2, temperature, and humidity.
 * Data is timestamped using RTC and stored on SD card.
 *
 * License: MIT License
 */

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <SoftwareSerial.h>
#include "MHZ19-SOLDERED.h"
#include <AHT10.h>

// -------------------- Configuration --------------------
#define LOG_INTERVAL 2000
#define SYNC_INTERVAL 1000

#define METHANE_PIN A0
#define SD_CS_PIN 10

#define LED_RED 2
#define LED_GREEN 3

// -------------------- Sensors --------------------
SoftwareSerial co2Serial(8, 9);
MHZ19 co2Sensor(&co2Serial);
AHT10Class aht;
RTC_DS1307 rtc;

// -------------------- File --------------------
File logfile;
uint32_t lastSync = 0;

// -------------------- Utility --------------------
void error(const char *msg) {
  Serial.print("ERROR: ");
  Serial.println(msg);
  digitalWrite(LED_RED, HIGH);
  while (1);
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  co2Serial.begin(9600);

  if (!aht.begin(eAHT10Address_Low)) {
    error("AHT10 failed");
  }

  if (!rtc.begin()) {
    error("RTC failed");
  }

  if (!SD.begin(SD_CS_PIN)) {
    error("SD card failed");
  }

  // Create unique file
  char filename[] = "LOG000.CSV";
  for (int i = 0; i < 1000; i++) {
    sprintf(filename, "LOG%03d.CSV", i);
    if (!SD.exists(filename)) {
      logfile = SD.open(filename, FILE_WRITE);
      break;
    }
  }

  if (!logfile) {
    error("File creation failed");
  }

  Serial.print("Logging to: ");
  Serial.println(filename);

  logfile.println("DateTime,Methane,CO2,Humidity,Temperature,DewPoint");
  Serial.println("DateTime,Methane,CO2,Humidity,Temperature,DewPoint");
}

// -------------------- Loop --------------------
void loop() {
  delay(LOG_INTERVAL);

  digitalWrite(LED_GREEN, HIGH);

  DateTime now = rtc.now();

  // Read sensors
  int methane = analogRead(METHANE_PIN) - 200;

  MHZ19_RESULT response = co2Sensor.retrieveData();
  int co2 = (response == MHZ19_RESULT_OK) ? co2Sensor.getCO2() : -1;

  float humidity = aht.GetHumidity();
  float temperature = aht.GetTemperature();
  float dewPoint = aht.GetDewPoint();

  // Log data
  logfile.print(now.timestamp());
  logfile.print(",");
  logfile.print(methane);
  logfile.print(",");
  logfile.print(co2);
  logfile.print(",");
  logfile.print(humidity);
  logfile.print(",");
  logfile.print(temperature);
  logfile.print(",");
  logfile.println(dewPoint);

  // Serial output
  Serial.print(now.timestamp());
  Serial.print(",");
  Serial.print(methane);
  Serial.print(",");
  Serial.print(co2);
  Serial.print(",");
  Serial.print(humidity);
  Serial.print(",");
  Serial.print(temperature);
  Serial.print(",");
  Serial.println(dewPoint);

  digitalWrite(LED_GREEN, LOW);

  // Sync SD card
  if (millis() - lastSync > SYNC_INTERVAL) {
    digitalWrite(LED_RED, HIGH);
    logfile.flush();
    digitalWrite(LED_RED, LOW);
    lastSync = millis();
  }
}
