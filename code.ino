// Author: Goda Gutparakyte
// Assignment: HW 3
// Last updated: 2025 11 04
// Server rack monitor made using ESP8266

#include "DHT.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "...";
const char* password = "...";

ESP8266WebServer server(80);  

int latch_pin = 4;   // D2
int clock_pin = 14;  // D5
int data_pin  = 13;  // D7
int temp_pin = 12; // D4
int relay_pin = 5; // D1

byte leds = 0;

float tempNormalMax = 27.0;
float tempWarningMax = 32.0;
float humNormalMax = 60.0;
float humWarningMax = 70.0;

int light_trigger = 800;

bool intrusion = false;
unsigned long last_blink = 0;
int blink_state = 0;
unsigned long blink_interval = 200;

float temperature;
float humidity;
int ldr_value;

DHT dht(temp_pin, DHT11);

void setup() {
  pinMode(latch_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, LOW);
  Serial.begin(9600);
  dht.begin();  
  delay(2000);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  // Start server
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  ldr_value = analogRead(A0);
  intrusion = (ldr_value < light_trigger);

  if(intrusion){
    if(millis() - last_blink >= blink_interval){
      blink_state = !blink_state;
      last_blink = millis();
      leds = blink_state? 0xFF : 0x00;
      update_shift_register();
    }
  }
  else{
    if (temperature <= tempNormalMax && humidity <= humNormalMax) {
        // Normal > green
        leds = 0b11100000; 
        digitalWrite(relay_pin, LOW);

    } else if (temperature <= tempWarningMax && humidity <= humWarningMax) {
        // Warning -> orange
        leds = 0b00011000; 
          digitalWrite(relay_pin, LOW);

    } else {
        // Critical -> red
        leds = 0b00000111; 
        digitalWrite(relay_pin, HIGH);
    }
    update_shift_register();

  }

  // update html
  server.handleClient();  
}

void update_shift_register() {
  digitalWrite(latch_pin, LOW);
  shiftOut(data_pin, clock_pin, LSBFIRST, leds);
  digitalWrite(latch_pin, HIGH);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Rack Monitor</title></head><body>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<h1>Server Rack Monitor</h1>";
  html += "<p>Temperature: " + String(temperature) + " C</p>";
  html += "<p>Humidity: " + String(humidity) + " %</p>";
  html += "<p>LDR Value: " + String(ldr_value) + "</p>";
  html += "<p>Intrusion: " + String(intrusion ? "YES" : "NO") + "</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}
