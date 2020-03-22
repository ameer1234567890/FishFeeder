#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

/* Secrets.h file should contain data as below: */
#ifndef STA_SSID
#define STA_SSID "your-ssid" // your network SSID (name)
#define STA_PSK  "your-password" // your network password
#endif

#define SERVO_PIN 2

bool opened = false;
Servo servo;
ESP8266WebServer server(80);

void setup() {
  Serial.begin (115200);
  Serial.println("Startup");
  servo.attach(SERVO_PIN);
  delay(1);
  servo.write(0);
  delay(500);
  servo.detach();
  delay(2000);
  server.on("/", []() {
    server.send(200, "text/html", \
      "<a href=\"/feed\">/feed</a>"\
    );
  });
  server.on("/feed", []() {
    server.send(200, "text/plain", "Feeding now!");
    feed();
  });
  server.begin();
  setupWifi();
}


void loop() {
  server.handleClient();
}


void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(STA_SSID, STA_PSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
    Serial.println(WiFi.localIP().toString());
}


void feed() {
  servo.attach(SERVO_PIN);
  delay(1);
  servo.write(180);
  delay(1500);
  servo.write(0);
  delay(500);
  servo.detach();
}