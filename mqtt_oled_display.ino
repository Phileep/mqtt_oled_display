// 
// OLED display setup from an ESP8266
// Accepts MQTT from the topic defined below as commandTopic
// expects JSON in the MQTT payload in format {"1":"message to show","2":"line2 message"} (second line optional)
// first item is line no - valid values are 1-3 (line no),
//                        0 Top line - pushing others down,
//                        9 bottom line - pushing others up




/// user constants inputs

 const char *ssid         = "mySSID";
 const char *password     = "password";
 const char* mqtt_server = "10.1.1.235";
 char* client_name = "oled"; // production version client name
 const char* commandTopic = "ha/oled1/message";  // command topic ESP will subscribe to and show on screen - 
 const char* willTopic = "ha/oled1/status";
 boolean willRetain = true;
 const char* willMessage = "offline" ;

 

// Arduino JSON copyright bit
// Copyright Benoit Blanchon 2014-2016
// MIT License
//
// Arduino JSON library
// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>
  DynamicJsonBuffer  jsonBuffer; // allocate dynamic buffer for json work
  JsonObject& root = jsonBuffer.createObject(); // create object for json maniupulation

#include <PubSubClient.h>


/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 by Daniel Eichhorn
 * Copyright (c) 2016 by Fabrice Weinberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// WiFi includes
 #include <ESP8266WiFi.h>

 // OTA Includes
 #include <ESP8266mDNS.h>
 #include <ArduinoOTA.h>


 boolean updatedMessage = false; // test if we have incoming data for update

WiFiClient espClient;
PubSubClient client(espClient);


char* strPayload = "";


 boolean updatedisplay = true; //if true will run the refresh of the screen

 String line1 = "";
 String line2 = ""; 
 String line3 = ""; 

// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
// or #include "SH1106.h" alis for `#include "SH1106Wire.h"`
// For a connection via I2C using brzo_i2c (must be installed) include
// #include <brzo_i2c.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Brzo.h"
// #include "SH1106Brzo.h"
// For a connection via SPI include
// #include <SPI.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Spi.h"
// #include "SH1106SPi.h"

// Use the corresponding display class:

// Initialize the OLED display using SPI
// D5 -> CLK
// D7 -> MOSI (DOUT)
// D0 -> RES
// D2 -> DC
// D8 -> CS
// SSD1306Spi        display(D0, D2, D8);
// or
// SH1106Spi         display(D0, D2);

// Initialize the OLED display using brzo_i2c
// D3 -> SDA
// D5 -> SCL
// SSD1306Brzo display(0x3c, D3, D5);
// or
// SH1106Brzo  display(0x3c, D3, D5);

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, D3, D5);
// SH1106 display(0x3c, D3, D5);

void setup_wifi() {
  delay(10);
  // Connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
    {delay(500); Serial.print(".");}

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  line1 = "IP : ";
  line1 += WiFi.localIP().toString();
}



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection as ...");
    Serial.print(client_name);
    Serial.print("..");
    // Attempt to connect
    if (client.connect(client_name, willTopic, 0, willRetain, willMessage)) {
      Serial.println("connected");
      // Once connected, update status to online - will Message will drop in if we go offline ...
      client.publish(willTopic,"online",true); 
          
      client.subscribe(commandTopic);// subscribe to the command topic - will listen here for comands to the RFLink
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // allocate buffer for ArduinoJson
//  DynamicJsonBuffer  jsonBuffer; // allocate dynamic buffer for json work
//  JsonObject& root = jsonBuffer.createObject(); // create object for json maniupulation

  Serial.begin(115200);
  
  display.init();
  display.flipScreenVertically();
  display.setContrast(255);
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
//  display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 10, "beginning wifi connection to :"  );
  display.drawStringMaxWidth(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 10, 128, "beginning wifi connection to :");
  display.display();

    setup_wifi();   
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 10, "OTA Update");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.drawProgressBar(4, 32, 120, 8, progress / (total / 100) );
    display.display();
  });

  ArduinoOTA.onEnd([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, "Restart");
    display.display();
  });
   
  updatedisplay = true; // trigger redraw of the screen in the loop


}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) {reconnect();}
  client.loop();

  if (updatedMessage) {UpdateMessages();}
  if (updatedisplay) {DoUpdateDisplay();} 
}

void DoUpdateDisplay() {
    updatedisplay = false;  
    
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    // line 1
    display.setFont(fontsetter(line1.length())); // set font size based upon line length
    display.drawStringMaxWidth(0, 0, 128, line1);

    // line 2
    display.setFont(fontsetter(line2.length())); // set font size based upon line length
    display.drawStringMaxWidth(0, 20, 128, line2);
    
    // line 3
    display.setFont(fontsetter(line3.length())); // set font size based upon line length
    display.drawStringMaxWidth(0, 42, 128, line3);
    
    display.display();
}

const char* fontsetter(int LineLengthToTest){
  if(LineLengthToTest > 16) 
    {return ArialMT_Plain_10; } // drop font size to fit in long text
  else {return ArialMT_Plain_16; } // otherwise we can use the bigger font
}

void callback(char* topic, byte* payload, unsigned int length) {
//  payload[length] = '\0'; // terminate payload

strPayload="";
strPayload = ((char*)payload);

Serial.println("got a message in");
Serial.println(strPayload);
updatedMessage=true;

}

void UpdateMessages() {
  // run when we have updated messages to prepare for use on the screen
  updatedMessage = false; // set flag to false so wont run until anoother message recieved
  
  JsonObject& root = jsonBuffer.parseObject(strPayload);
  // Test if parsing succeeds.
  if (!root.success()) {   
    return;} // didnt work - jump out of here!

  Serial.println("processing json");
  updatedisplay = true; 
  
  if (root.containsKey("1")) // line 1 message
    {line1 = root["1"].asString();}
  if (root.containsKey("2")) // line 2 message
    {line2 = root["2"].asString();}
  if (root.containsKey("3")) // line 3 message
    {line3 = root["3"].asString();}

  if (root.containsKey("0")) // line 0 - means force to top and push other lines down
    {
      //strcpy( line3 , line2 );
      line3 = line2;
      //strcpy( line2 , line1 );
      line2 = line1;
      line1 = root["0"].asString();
    }

  if (root.containsKey("9")) // line 9 - meas force to bottom and push other lines up
    {
      line2 = line3;
      line3 = root["9"].asString();
    } 
}

