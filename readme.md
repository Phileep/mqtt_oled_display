Arduino based ESP8266 MQTT driven OLED display

Uses 0.96" OLED display attached to ESP8266

Tested on NodeMCU V1.0
  
Accepts MQTT from the topic defined below as commandTopic
 expects JSON in the MQTT payload in format {"1":"message to show","2":"line2 message"} (second line optional)
 first item is line no - valid values are 1-3 (line no),
                        0 Top line - pushing others down,
                        9 bottom line - pushing others up


  
Phil Wilson October 2017
