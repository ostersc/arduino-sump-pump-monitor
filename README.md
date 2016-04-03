# arduino-sump-pump-monitor
Arduino project for ESP8266 to monitor backup sump pump and water level, sending alerts upon issues.

The project monitors my sump pump for when its backup pump engages, or when the main pump is running on battery power (due to power outage).  The project continually monitors the pumps, reporting data to thingspeak.com indicating the pump status.  The board is equiped with a LiPo battery backup, so it is still able to report in the event of a power outage.
![Graph](http://i.imgur.com/ybL46LY.png)

## PUMP
I have the Basement Watchdog BW4000 Sump Pump, which has a backup pump, battery, and remote terminal to indicate alarms.
http://www.basementwatchdog.com/Basement_Watchdog_Combination_Systems.php
http://www.basementwatchdog.com/Accessories/Remote_Terminal.php
Any similar system could be used, or a homebrew sensor could be created, as all this "remote terminal" does is provide an open/closed circuit transition when triggering an alarm.

## HARDWARE
I'm using the Sparkfun ESP8266 Thing (https://www.sparkfun.com/products/13231)
The Adafruit Feather HUZZAH actually looks easier to program; it should work as well (https://www.adafruit.com/products/2821)

## CURRENT STATUS
*  Sump alarm monitoring (backup pump used, power loss, fuse, or battery issue)
*  Reporting to thingspeak (signal strength, alarm state, alarm duration, alarm count)
* Email when alarm state entered (via https://temboo.com/arduino/others/send-an-email)

## TODO
* Water level alarm (float switch or sonar level threshold)
* Battery level monitoring (and maybe deep sleep) (via https://learn.adafruit.com/using-ifttt-with-adafruit-io/wiring#battery-tracking)
* Collect settings and alert email via the wifi capture screen (via https://github.com/tzapu/WiFiManager#custom-parameters)
* 3D print an enclosure

## LIBRARIES
* https://github.com/tzapu/WiFiManager
* https://github.com/mathworks/thingspeak-arduino

## SETUP
- Create a file called Secrets.h, copying the commented out block from the ino sketch
- Create a thingspeak channel [here](https://thingspeak.com/)
  - The fields should be ```id alarm alarmduration alarmcount rssi time```
  - Edit the myChannelNumber and myWriteAPIKey in Secrets.h
  - ![Channel Setup](http://i.imgur.com/DXxmMLK.png)
- Follow instructions on setting up Temboo account and gmail [here](https://temboo.com/arduino/others/send-an-email)
  - copy all the TEMBOO settings from the header section (normally Temboo.h) and put in Secrets.h
- You can setup additional alarms as needed (such as no data alarms, low wifi strength, etc) by using thingspeaks ability to trigger IFTTT events [as described here](http://www.makeuseof.com/tag/ifttt-connect-anything-maker-channel/).
