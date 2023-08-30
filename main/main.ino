#include "M5Atom.h"
#include <WiFi.h>
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"
#include <FastLED.h>

const long timeoutTime = 2000;
const char* ssid = "ESP32-Access-Point";
const char* password = "12345678";
const int output32 = 32;

CRGB leds[60];
WiFiServer server(80);
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
String header;
String output32State = "off";
unsigned long currentTime = millis();
unsigned long previousTime = 0;
uint8_t brightnest = 75;
CRGB color = CRGB(255, 0, 0);

void setup() {
  M5.begin(true, true, true);
  delay(50);
  M5.dis.fillpix(0x0000ff);
  if (tsl.begin()) {
    tsl.setGain(TSL2591_GAIN_MED);
    tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  }

  FastLED.addLeds<WS2812, 32, GRB>(leds, 60);
  WiFi.softAP(ssid, password);
  server.begin();
  M5.dis.fillpix(0xff0000);
}

void loop() {
  WiFiClient client = server.available();  // Listen for incoming clients

  if (client) {  // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");                                             // print a message out in the serial port
    String currentLine = "";                                                   // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        Serial.write(c);         // print it out the serial monitor
        header += c;
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /on") >= 0) {
              output32State = "on";
              M5.dis.fillpix(0x33ff00);
              setLedsOn();
            } else if (header.indexOf("GET /off") >= 0) {
              output32State = "off";
              M5.dis.fillpix(0xff0000);
              setLedsOff();
            } else if (header.indexOf("GET /brightnest-add") >= 0) {
              brightnest += 25;
              FastLED.setBrightness(brightnest);
              FastLED.show();
            } else if (header.indexOf("GET /brightnest-substract") >= 0) {
              brightnest -= 25;
              FastLED.setBrightness(brightnest);
              FastLED.show();
            } else if (header.indexOf("GET /blue") >= 0) {
              color = CRGB(0, 0, 255);
              setLedsOn();
            } else if (header.indexOf("GET /green") >= 0) {
              color = CRGB(0, 255, 0);
              setLedsOn();
            } else if (header.indexOf("GET /red") >= 0) {
              color = CRGB(255, 0, 0);
              setLedsOn();
            } else if (header.indexOf("GET /orange") >= 0) {
              color = CRGB(255, 128, 0);
              setLedsOn();
            } else if (header.indexOf("GET /purple") >= 0) {
              color = CRGB(153, 0, 255);
              setLedsOn();
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;}");
            client.println(".color-button { border: none; color: white; padding: 16px 16px;}");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");

            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");

            // Display current state, and ON/OFF buttons for GPIO 26
            client.println("<p>LED - State " + output32State + "</p>");
            client.println("<p>lux: " + String(getLux()) + "</p>");

            // Color buttons
            client.println("<a href=\"/blue\"><button class=\"color-button\" style=\"background-color: #0000FF\"></button></a>");
            client.println("<a href=\"/green\"><button class=\"color-button\" style=\"background-color: #00FF00\"></button></a>");
            client.println("<a href=\"/red\"><button class=\"color-button\" style=\"background-color: #FF0000\"></button></a>");
            client.println("<a href=\"/orange\"><button class=\"color-button\" style=\"background-color: #FF8000\"></button></a>");
            client.println("<a href=\"/purple\"><button class=\"color-button\" style=\"background-color: #9900FF\"></button></a>");

            // If the output26State is off, it displays the ON button
            if (output32State == "off") {
              client.println("<p><a href=\"/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            if (brightnest >= 255) {
              client.println("<p><button class=\"button button2\">ON</button></p>");
            } else {
              client.println("<p><a href=\"/brightnest-add\"><button class=\"button\">Brightnest +</button></a></p>");
            }

            if (brightnest <= 1) {
              client.println("<p><button class=\"button button2\">MIN</button></p>");
            } else {
              client.println("<p><a href=\"/brightnest-substract\"><button class=\"button\">Brightnest -</button></a></p>");
            }

            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else {  // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
  }
}

float getLux() {
  uint32_t tsl2591_data = tsl.getFullLuminosity();  // Get CH0 & CH1 data from the sensor (two 16-bit registers)

  uint16_t ir, ir_visible;
  ir = tsl2591_data >> 16;             // extract infrared value
  ir_visible = tsl2591_data & 0xFFFF;  // extract visible + infrared value

  float lux = tsl.calculateLux(ir_visible, ir);  // Calculate light lux value

  return lux;
}

void setLedsOn() {
  for (int i = 0; i <= 60; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

void setLedsOff() {
  for (int i = 0; i <= 60; i++) {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
}