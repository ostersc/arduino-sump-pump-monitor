
#include <ESP8266WiFi.h>
#include <Ticker.h>
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
const int WAIT_SECS_FOR_WIFI = 60;

//Thing's onboard LED
const int LED_PIN = 5;
// other pings available on Thing
const int ANALOG_PIN = A0;
//const int DIGITAL_PIN_1 = 0;
//const int DIGITAL_PIN_2 = 4;
//const int DIGITAL_PIN_3 = 12;
//const int DIGITAL_PIN_4 = 13;
const int ALARM_PIN = 4;

const int NUM_READINGS = 100;

int moistureAverage = 0;
int moistureState = LOW;
int moistureAlarmCount = 0;
const int moistureThreshold = 250;
unsigned long lastMoistureAlarm = 0;

unsigned long lastAlarm = 0;
int alarmState = LOW;
int alarmCount = 0;
String macAddress;

// Time to sleep (in seconds):
//const int DEEP_SLEEP_SECS = 30;
//10 mins
const int DEEP_SLEEP_SECS = 10*60;

// Use WiFiClient class to create TCP connections
WiFiClient client;

//comment out to disable debug output
//#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(x)  Serial.print (x)
#define DEBUG_PRINTLN(x)  Serial.println (x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

Ticker ticker;

void tick()
{
  //toggle state
  int state = digitalRead(LED_PIN);
  digitalWrite(LED_PIN, !state);
}


void configModeCallback (WiFiManager *myWiFiManager) {
  DEBUG_PRINTLN("Unable to connect to configured WiFi network.");
  DEBUG_PRINTLN("Entered config mode with network:");
  DEBUG_PRINTLN(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());
  ticker.attach(0.2, tick);
}

void setupPins() {
  //  pinMode(DIGITAL_PIN_1, INPUT_PULLUP);
  //  pinMode(DIGITAL_PIN_2, INPUT_PULLUP);
  //  pinMode(DIGITAL_PIN_3, INPUT_PULLUP);
  //  pinMode(DIGITAL_PIN_4, INPUT_PULLUP);
  pinMode(ALARM_PIN, INPUT_PULLUP);
  pinMode(ANALOG_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void goToSleep() {
  DEBUG_PRINTLN("Going to sleep...");
  // deepSleep time is defined in microseconds. Multiply seconds by 1e6
  ESP.deepSleep(DEEP_SLEEP_SECS * 1000000);
}

int setupWiFi() {
  WiFiManager wifiManager;

  //reset settings - for testing captive wifi
  //wifiManager.resetSettings();

  //sets timeout until configuration portal gets turned off; want enough time so we can program wifi when needed, but not waste power when power is out
  //TODO: maybe add a button to
  wifiManager.setTimeout(WAIT_SECS_FOR_WIFI);

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(SSID)) {
    DEBUG_PRINTLN("failed to connect and hit timeout!");
    return -1;
  }

  macAddress = WiFi.macAddress();

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected");
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
  DEBUG_PRINTLN("MAC: ");
  DEBUG_PRINTLN(macAddress);

  ThingSpeak.begin(client);
  return 1;
}

void sendSumpAlert(String alert) {
  //  DEBUG_PRINTLN("Sending alert email!");
  //  TembooChoreo SendEmailChoreo(client);
  //
  //  // Invoke the Temboo client
  //  SendEmailChoreo.begin();
  //
  //  // Set Temboo account credentials
  //  SendEmailChoreo.setAccountName(TEMBOO_ACCOUNT);
  //  SendEmailChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  //  SendEmailChoreo.setAppKey(TEMBOO_APP_KEY);
  //
  //  // Set Choreo inputs
  //  SendEmailChoreo.addInput("CC", EMAIL_CC);
  //  SendEmailChoreo.addInput("FromAddress", EMAIL_FROM);
  //  SendEmailChoreo.addInput("Username", EMAIL_USER);
  //  SendEmailChoreo.addInput("ToAddress", EMAIL_TO);
  //  SendEmailChoreo.addInput("Subject", alert);
  //  SendEmailChoreo.addInput("Password", EMAIL_PASSWORD);
  //  //TODO: add current values here
  //  SendEmailChoreo.addInput("MessageBody", alert);
  //
  //  // Identify the Choreo to run
  //  SendEmailChoreo.setChoreo("/Library/Google/Gmail/SendEmail");
  //
  //  // Run the Choreo; when results are available, print them to serial
  //  SendEmailChoreo.run();
  //
  //  while (SendEmailChoreo.available()) {
  //    char c = SendEmailChoreo.read();
  //    DEBUG_PRINT(c);
  //  }
  //  SendEmailChoreo.close();
  //  delay(250);
  //  if (client.connected()) {
  //    client.stop(); // stop() closes a TCP connection.
  //    DEBUG_PRINTLN("Connection closed.");
  //  } else {
  //    DEBUG_PRINTLN("Connection was not active.");
  //  }
}

void updateMoistureLevel() {
  int moistureTotal = 0;
  for (int i = 0; i < NUM_READINGS; i++) {
    moistureTotal += analogRead(ANALOG_PIN);
    delay(100);
  }
  moistureAverage = moistureTotal / NUM_READINGS;
  DEBUG_PRINT("Moisture avg:  ");
  DEBUG_PRINTLN(moistureAverage);

  if (moistureState == LOW) {
    //if we are in non alarm state, and the threshold is exceeded, go to alarm state
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
    //if we are in alarm state, and the threshold is not exceeded, go to non alarm state
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
}

int checkAndReport() {
  DEBUG_PRINTLN("Sending data.");

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

  ThingSpeak.setField(1, macAddress);
  ThingSpeak.setField(2, alarmState);
  ThingSpeak.setField(3, alarmDuration);
  ThingSpeak.setField(4, alarmCount);
  ThingSpeak.setField(5, moistureAverage);
  ThingSpeak.setField(6, moistureAlarmCount);
  ThingSpeak.setField(7,  moistureAlarmDuration);
  ThingSpeak.setField(8,  WiFi.RSSI());
  DEBUG_PRINTLN("Sending data thingspeak.");
  
  // Write the fields that you've set all at once.
  return ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
}

void setup() {
  Serial.begin(9600);
  delay(100);
  DEBUG_PRINTLN("STARTING UP");

  setupPins();

  ticker.attach(0.5, tick);

  if (!setupWiFi()) {
    //TODO: wifi failed, don't try to report or send alarms
    //probably means power is out and we are on battery... what to do?
    DEBUG_PRINTLN("UNABLE TO CONNECT TO WIFI!");
  }

  ticker.detach();

  //readFromFlash();

  DEBUG_PRINTLN("Collecting moisture levels.");
  updateMoistureLevel();

  DEBUG_PRINTLN("Checking alarms and reporting data.");
  digitalWrite(LED_PIN, HIGH);
  int result=checkAndReport() ;
  digitalWrite(LED_PIN, LOW);
  
  if (result== OK_SUCCESS) {
    DEBUG_PRINTLN("Success! Going to sleep.");
  } else {
    //TODO: if result isn't success, retry a few times, then record data to flash
    DEBUG_PRINTLN("FAILURE TO SEND DATA! Going to sleep anyway.");
    ticker.attach(0.1, tick);
    delay(5000);
    ticker.detach();

  }
  //if alarm values changed from flash values, write them back out to flash (maybe including if we were able to send or not?)
  // want to make sure that state transitions that occur when no internet, are properly reported
  // e.g  OK -> offline -> ALARM -> online (with no state transition)
  // so send alert if transition, or if last report state to current is a transition

  goToSleep();
}


void loop() {
  //never gets here due to sleep mode
}

