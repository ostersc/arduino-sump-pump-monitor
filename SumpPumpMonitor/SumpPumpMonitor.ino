
#include <ESP8266WiFi.h>
#include <Phant.h>  // https://github.com/sparkfun/phant-arduino
#include "WiFiManager.h"  //https://github.com/tzapu/WiFiManager

// Phant detsination server:
const char phantServer[] = "data.sparkfun.com";
// Phant public key:
const char publicKey[] = "LQJZlmzWDlTNl7XnnWW4";
// Phant private key:
const char privateKey[] = "REDACTED";
// Create a Phant object, which we'll use from here on:
Phant phant(phantServer, publicKey, privateKey);

const char* SSID = "SumpPumpMonitor";

//Thing's onboard LED
const int LED_PIN = 5;
// other pings available on Thing
//const int ANALOG_PIN = A0;
//const int DIGITAL_PIN_1 = 0;
//const int DIGITAL_PIN_2 = 4;
//const int DIGITAL_PIN_3 = 12;
//const int DIGITAL_PIN_4 = 13;
const int ALARM_PIN = 4;

//phant only lets report every 10 seconds or so
const unsigned long postRate = 10000;
unsigned long lastPost = 0;
int alarmState = LOW;
unsigned long lastAlarm = 0;
String macAddress;

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
}

int postToPhant() {
  DEBUG_PRINT("Sending data.");

  int oldAlarmState = alarmState;
  alarmState = digitalRead(ALARM_PIN);
  if (oldAlarmState != alarmState) {
    if (alarmState == HIGH) {
      lastAlarm = millis();
      DEBUG_PRINT("Alarm detected at ");
      DEBUG_PRINTLN(lastAlarm);
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

  // LED turns on when we enter, it'll go off when we
  // successfully post.
  digitalWrite(LED_PIN, HIGH);

  phant.add("id", macAddress);
  phant.add("alarm", alarmState);
  phant.add("alarmduration", alarmDuration);
  phant.add("rssi", WiFi.RSSI());
  phant.add("time", millis());

  // Use WiFiClient class to create TCP connections
  WiFiClient client;

  DEBUG_PRINT("Sending data to ");
  DEBUG_PRINTLN(phant.url());

  if (client.connect(phantServer, 80) <= 0) {
    DEBUG_PRINTLN("Failed to connect to server.");
    return 0;
  }
  DEBUG_PRINTLN("Connection Established.");
  client.println(phant.post());

  delay(1000);
  while (client.available()) {
    String line = client.readStringUntil('\r');
    DEBUG_PRINT(line);
  }
  DEBUG_PRINTLN();

  // Before we exit, turn the LED off.
  digitalWrite(LED_PIN, LOW);
  return 1;
}

void loop() {
  if (lastPost + postRate <= millis()) {
    if (postToPhant()) {
      lastPost = millis();
    } else {
      delay(100);
    }
  }
}
