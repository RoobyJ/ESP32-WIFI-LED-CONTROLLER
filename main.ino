#include "M5Atom.h"
#include <WiFi.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"

// Set web server port number to 80
WiFiServer server(80);

Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output32State = "off";

// Assign output variables to GPIO pins
const int output32 = 32;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
const char* ssid = "ESP32-Access-Point";
const char* password = "12345678";


void setup() {
  M5.begin(true, false, true);
  delay(50);
  M5.dis.fillpix(0x0000ff);
  pinMode(output32, OUTPUT);
  if (tsl.begin()) {
    tsl.setGain(TSL2591_GAIN_MED);
    tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  }

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
              Serial.println("GPIO 32 on");
              output32State = "on";
              M5.dis.fillpix(0x33ff00);
              digitalWrite(output32, HIGH);
            } else if (header.indexOf("GET /off") >= 0) {
              Serial.println("GPIO 32 off");
              output32State = "off";
              M5.dis.fillpix(0xff0000);
              digitalWrite(output32, LOW);
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");

            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");

            // Display current state, and ON/OFF buttons for GPIO 26
            client.println("<p>LED - State " + output32State + "</p>");
            client.println("<p>lux: " + String(getLux()) + "</p>");
            // If the output26State is off, it displays the ON button
            if (output32State == "off") {
              client.println("<p><a href=\"/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/off\"><button class=\"button button2\">OFF</button></a></p>");
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
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

float getLux() {
  uint32_t tsl2591_data = tsl.getFullLuminosity();  // Get CH0 & CH1 data from the sensor (two 16-bit registers)

  uint16_t ir, ir_visible;
  ir = tsl2591_data >> 16;             // extract infrared value
  ir_visible = tsl2591_data & 0xFFFF;  // extract visible + infrared value

  float lux = tsl.calculateLux(ir_visible, ir);  // Calculate light lux value
  Serial.println(lux);
  return lux;
}