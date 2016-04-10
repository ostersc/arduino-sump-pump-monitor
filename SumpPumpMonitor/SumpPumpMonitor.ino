
#include <ESP8266WiFi.h>
#include "WiFiManager.h"  //https://github.com/tzapu/WiFiManager
#include "Temboo.h"  //https://temboo.com/library/ https://github.com/arduino-libraries/Temboo
#include "ThingSpeak.h" //https://github.com/mathworks/thingspeak-arduino
#include "Secrets.h"
/**
   COPY THESE INTO A FILE CALLED Secrets.h AND REPLACE THE VALUES FOR YOUR ACCOUNT
  #define TEMBOO_ACCOUNT "your account"  // Your Temboo account name
  #define TEMBOO_APP_KEY_NAME "myFirstApp"  // Your Temboo app key name
  #define TEMBOO_APP_KEY "your key"  // Your Temboo app key

  #define EMAIL_USER "example@gmail.com"
  #define EMAIL_PASSWORD "asdfasdfasdfasdfasdf"
  #define EMAIL_TO "example@gmail.com"
  #define EMAIL_CC "example@gmail.com"
  #define EMAIL_FROM "example@gmail.com"

  unsigned long myChannelNumber = 123456;
  const char * myWriteAPIKey = "yourapikey";
*/

#define ALARM_EMAIL_SUBJECT "Sump Pump Alarm Triggered!"
#define WATER_EMAIL_SUBJECT "Water in Sump Pump!"

const char* SSID = "SumpPumpMonitor";

//Thing's onboard LED
const int LED_PIN = 5;
// other pings available on Thing
const int ANALOG_PIN = A0;
//const int DIGITAL_PIN_1 = 0;
//const int DIGITAL_PIN_2 = 4;
//const int DIGITAL_PIN_3 = 12;
//const int DIGITAL_PIN_4 = 13;
const int ALARM_PIN = 4;

const int NUM_READINGS = 20;

int moistureReadings[NUM_READINGS];   // the readings from the analog input
int moistureReadIndex = 0;            // the index of the current reading
int moistureTotal = 0;                // the running total
int moistureAverage = 0;              // the average
int moistureState = LOW;
int moistureAlarmCount = 0;
const int moistureThreshold = 250;
unsigned long lastMoistureAlarm = 0;
#define DEBUG_MOISTURE false

//thingspeak only lets report every 15 seconds or so
const unsigned long postRate = 15000;
unsigned long lastPost = 0;
unsigned long lastAlarm = 0;
int alarmState = LOW;
int alarmCount = 0;
String macAddress;

// Use WiFiClient class to create TCP connections
WiFiClient client;

//comment out to disable debug output
//#define DEBUG
//WARNING: printing to the serial port while sending data over wifi, corrupts the data (at least with sparkfun's board posting to phant)

#ifdef DEBUG
#define DEBUG_PRINT(x)  Serial.print (x)
#define DEBUG_PRINTLN(x)  Serial.println (x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif


void configModeCallback (WiFiManager *myWiFiManager) {
  DEBUG_PRINTLN("Entered config mode");
  DEBUG_PRINTLN(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  Serial.begin(9600);
  delay(100);
  DEBUG_PRINTLN();

  //  pinMode(DIGITAL_PIN_1, INPUT_PULLUP);
  //  pinMode(DIGITAL_PIN_2, INPUT_PULLUP);
  //  pinMode(DIGITAL_PIN_3, INPUT_PULLUP);
  //  pinMode(DIGITAL_PIN_4, INPUT_PULLUP);
  pinMode(ALARM_PIN, INPUT_PULLUP);
  pinMode(ANALOG_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFiManager wifiManager;

  //reset settings - for testing captive wifi
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(SSID)) {
    DEBUG_PRINTLN("failed to connect and hit timeout");
    DEBUG_PRINT(".");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  macAddress = WiFi.macAddress();

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected");
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
  DEBUG_PRINTLN("MAC: ");
  DEBUG_PRINTLN(macAddress);

  ThingSpeak.begin(client);
}

void sendSumpAlert(String alert) {
  DEBUG_PRINTLN("Sending alert email!");
  TembooChoreo SendEmailChoreo(client);

  // Invoke the Temboo client
  SendEmailChoreo.begin();

  // Set Temboo account credentials
  SendEmailChoreo.setAccountName(TEMBOO_ACCOUNT);
  SendEmailChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  SendEmailChoreo.setAppKey(TEMBOO_APP_KEY);

  // Set Choreo inputs
  SendEmailChoreo.addInput("CC", EMAIL_CC);
  SendEmailChoreo.addInput("FromAddress", EMAIL_FROM);
  SendEmailChoreo.addInput("Username", EMAIL_USER);
  SendEmailChoreo.addInput("ToAddress", EMAIL_TO);
  SendEmailChoreo.addInput("Subject", alert);
  SendEmailChoreo.addInput("Password", EMAIL_PASSWORD);
  //TODO: add current values here
  SendEmailChoreo.addInput("MessageBody", alert);

  // Identify the Choreo to run
  SendEmailChoreo.setChoreo("/Library/Google/Gmail/SendEmail");

  // Run the Choreo; when results are available, print them to serial
  SendEmailChoreo.run();

  while (SendEmailChoreo.available()) {
    char c = SendEmailChoreo.read();
    DEBUG_PRINT(c);
  }
  SendEmailChoreo.close();
  delay(250);
  if (client.connected()) {
    client.stop(); // stop() closes a TCP connection.
    DEBUG_PRINTLN("Connection closed.");
  } else {
    DEBUG_PRINTLN("Connection was not active.");
  }
}

void updateMoistureLevel() {
  int moistureLevel = analogRead(ANALOG_PIN);

  // subtract the last reading:
  moistureTotal = moistureTotal - moistureReadings[moistureReadIndex];
  // read from the sensor:
  moistureReadings[moistureReadIndex] = moistureLevel;
  // add the reading to the total:
  moistureTotal = moistureTotal + moistureReadings[moistureReadIndex];
  // advance to the next position in the array:
  moistureReadIndex += 1;

  // if we're at the end of the array...
  if (moistureReadIndex >= NUM_READINGS) {
    // ...wrap around to the beginning:
    moistureReadIndex = 0;
  }

  // calculate the average:
  moistureAverage = moistureTotal / NUM_READINGS;

  if (moistureState == LOW) {
    if (moistureAverage > moistureThreshold) {
      lastMoistureAlarm = millis();
      moistureAlarmCount++;
      moistureState = HIGH;
      DEBUG_PRINT("Water detected at ");
      DEBUG_PRINTLN(lastMoistureAlarm);
      DEBUG_PRINT("Moisture detected! The moisture level is: ");
      DEBUG_PRINTLN(moistureAverage);
      sendSumpAlert(WATER_EMAIL_SUBJECT);
    }
  } else {
    if (moistureAverage <= moistureThreshold ) {
      DEBUG_PRINT("Water alarm ended at ");
      DEBUG_PRINTLN(millis());
      DEBUG_PRINT("Lasting for ");
      DEBUG_PRINT((millis() - lastMoistureAlarm) / 1000);
      DEBUG_PRINTLN(" seconds.");
      DEBUG_PRINT("Moisture level below alarm threshold: ");
      DEBUG_PRINTLN(moistureAverage);
      moistureState = LOW;
    }
  }

  if (DEBUG_MOISTURE) {
    DEBUG_PRINT("\tCurrent moisture level:");
    DEBUG_PRINTLN(moistureLevel);
    DEBUG_PRINT("Moisture avg:  ");
    DEBUG_PRINTLN(moistureAverage);
  }
}


int checkAndReport() {
  DEBUG_PRINT("Sending data.");

  int oldAlarmState = alarmState;
  alarmState = digitalRead(ALARM_PIN);

  if (oldAlarmState != alarmState) {
    if (alarmState == HIGH) {
      lastAlarm = millis();
      alarmCount++;
      DEBUG_PRINT("Alarm detected at ");
      DEBUG_PRINTLN(lastAlarm);
      sendSumpAlert(ALARM_EMAIL_SUBJECT);
    } else if (alarmState == LOW) {
      DEBUG_PRINT("Alarm ended at ");
      DEBUG_PRINTLN(millis());
      DEBUG_PRINT("Lasting for ");
      DEBUG_PRINT((millis() - lastAlarm) / 1000);
      DEBUG_PRINTLN(" seconds.");
    }
  }

  int alarmDuration = 0;
  if (alarmState == HIGH) {
    alarmDuration = (millis() - lastAlarm) / 1000;
  }

  int moistureAlarmDuration = 0;
  if (moistureState == HIGH) {
    moistureAlarmDuration = (millis() - lastMoistureAlarm) / 1000;
  }

  // LED turns on when we enter, it'll go off when we
  // successfully post.
  digitalWrite(LED_PIN, HIGH);

  ThingSpeak.setField(1, macAddress);
  ThingSpeak.setField(2, alarmState);
  ThingSpeak.setField(3, alarmDuration);
  ThingSpeak.setField(4, alarmCount);
  ThingSpeak.setField(5, moistureAverage);
  ThingSpeak.setField(6, moistureAlarmCount);
  ThingSpeak.setField(7,  moistureAlarmDuration);
  ThingSpeak.setField(8,  WiFi.RSSI());
  DEBUG_PRINT("Sending data thingspeak.");
  // Write the fields that you've set all at once.
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  // Before we exit, turn the LED off.
  digitalWrite(LED_PIN, LOW);
  return 1;
}

void loop() {
  delay(100);
  if (millis() - lastPost >= postRate) {
    updateMoistureLevel();
    if (checkAndReport()) {
      lastPost = millis();
    } else {
      delay(2000);
    }
  }
}

