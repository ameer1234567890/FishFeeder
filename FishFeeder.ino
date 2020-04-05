#include <Time.h>
#include <Servo.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "Secrets.h"

/* Secrets.h file should contain data as below: */
#ifndef STA_SSID
#define STA_SSID "your-ssid" // your network SSID (name)
#define STA_PSK  "your-password" // your network password
#define OTA_HOSTNAME "FishFeeder" // desired hostname for OTA
#endif

/* Configurable variables */
#define SERVO_PIN 2
const int DST = 0;
const int TIMEZONE = 5;

/* Do not change these unless you know what you are doing */
Servo servo;
String logMsg;
String logTime;
bool fed = false;
uint eepromAddr = 0;
bool opened = false;
ESP8266WebServer server(80);
typedef struct { 
  uint set = 0;
  uint hour = 0;
  uint minute = 0;
} eepromData;
eepromData feedTimeData;
eepromData feedTimeDataOld;

void setup() {
  Serial.begin (115200);
  log("I/system: startup");
  servo.attach(SERVO_PIN);
  delay(1);
  servo.write(0);
  delay(500);
  servo.detach();
  delay(2000);

  EEPROM.begin(512);
  EEPROM.get(eepromAddr, feedTimeData);

  server.on("/", []() {
    server.send(200, "text/html", \
      "<a href=\"/app\">/app</a><br>"\
      "<a href=\"/log\">/log</a><br>"\
      "<a href=\"/reboot\">/reboot</a><br>"\
      "<a href=\"/feed\">/feed</a>"\
      "<a href=\"/setfeedtime?hour=14&minute=44\">/setfeedtime?hour=14&minute=44</a><br>"\
      "<a href=\"/showfeedtime\">/showfeedtime</a><br>"\
      "<a href=\"/cancelfeedtime\">/cancelfeedtime</a><br>"\
      "<br><p><small>"\
      "Powered by: <a href=\"https://github.com/ameer1234567890/FishFeeder\">FishFeeder</a> | "\
      "Chip ID: " + String(ESP.getChipId()) + \
      "</small></p>"\
    );
  });
  server.on("/app", []() {
    String setoneState;
    String timeoneState;
    if (feedTimeData.set == 0) {
      timeoneState = "disabled";
    } else {
      setoneState = "checked";
    }
    server.send(200, "text/html", \
      "<!DOCTYPE html>"\
      "<html>"\
      "<head>"\
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"\
      "<style>"\
      ".switch{position:relative;display:inline-block;width:60px;height:34px;}"\
      ".switch input{opacity:0;width:0;height:0;}"\
      ".slider{position:absolute;cursor: pointer;top:0;left:0;right:0;bottom:0;background-color:#ccc;transition:.4s;}"\
      ".slider:before{position:absolute;content:\"\";height:26px;width:26px;left:4px;bottom:4px;background-color:white;transition:.4s;}"\
      "input:checked+.slider{background-color:#2196F3;}"\
      "input:focus+.slider{box-shadow:0 0 1px #2196F3;}"\
      "input:checked+.slider:before{transform: translateX(26px);}"\
      ".slider.round{border-radius:34px;}"\
      ".slider.round:before{border-radius:50%;}"\
      ".spacer{padding-left:12px;}"\
      ".slotname{vertical-align:bottom;font-weight:bold;}"\
      ".timeselector{vertical-align:bottom;height:30px;}"\
      ".tickmark{vertical-align:bottom;font-size:25px;color:#2196F3;cursor:pointer;visibility:hidden;}"\
      ".tickmark.tick{animation:fadein 1s;}"\
      "@keyframes fadein{from{opacity:0;}to{opacity:1;}}"\
      "</style>"\
      "</head>"\
      "<body>"\
      "<h3>Fish Feeder</h3>"\
      "<label class=\"slotname\">SLOT 1:</label>"\
      "<span class=\"spacer\"></span>"\
      "<label class=\"switch\">"\
      "<input type=\"checkbox\" name=\"setone\" id=\"setone\" " + setoneState + ">"\
      "<span class=\"slider round\"></span>"\
      "</label>"\
      "<span class=\"spacer\"></span>"\
      "<input type=\"time\" id=\"timeone\" name=\"timeone\" "\
      "value=\"" + feedTimeData.hour + ":" + feedTimeData.minute + "\" class=\"timeselector\" " + timeoneState + ">"\
      "<span class=\"spacer\"></span>"\
      "<span class=\"tickmark\" id=\"tickone\">&#10148;</span>"\
      "<script>"\
      "setone=document.querySelector('#setone');"\
      "timeone=document.querySelector('#timeone');"\
      "tickone=document.querySelector('#tickone');"\
      "setone.onchange=function(e){"\
      "httpRequest=new XMLHttpRequest();"\
      "if(setone.checked){timeone.disabled='';"\
      "httpRequest.open('GET','/setfeedtime?hour='+timeone.value.substr(0,2)+'&minute='+timeone.value.substr(3,4));"\
      "httpRequest.send();"\
      "httpRequest.onreadystatechange=function(){"\
      "if(httpRequest.readyState==4){if(httpRequest.status!=200){setone.checked=false;timeone.disabled='disabled';}"\
      "}}"\
      "}else{timeone.disabled='disabled';httpRequest.open('GET','/cancelfeedtime');httpRequest.send();"\
      "httpRequest.onreadystatechange=function(){"\
      "if(httpRequest.readyState==4){if(httpRequest.status!=200){setone.checked=true;timeone.disabled='';}"\
      "}}"\
      "}};"\
      "timeone.oninput=function(e){"\
      "tickone.style.visibility='visible';tickone.classList.add('tick');setTimeout(function(){tickone.classList.remove('tick');},1000);"\
      "};"\
      "tickone.onclick=function(e){"\
      "httpRequest=new XMLHttpRequest();"\
      "tickone.style.color='#ccc';"\
      "httpRequest.open('GET','/setfeedtime?hour='+timeone.value.substr(0,2)+'&minute='+timeone.value.substr(3,4));"\
      "httpRequest.send();"\
      "httpRequest.onreadystatechange=function(){"\
      "if(httpRequest.readyState==4){if(httpRequest.status==200){"\
      "tickone.innerHTML='&#10004;';tickone.classList.add('tick');tickone.style.color='#34995f';"\
      "setTimeout(function(){tickone.innerHTML='&#10148;';tickone.classList.remove('tick');tickone.style.color='#2196F3';tickone.style.visibility='hidden';},1000);"\
      "}else{"\
      "tickone.classList.add('tick');"\
      "tickone.innerHTML='&#x26A0;';tickone.style.color='#F5BD1F';}"\
      "setTimeout(function(){tickone.innerHTML='';tickone.classList.remove('tick');},1000);"\
      "setTimeout(function(){tickone.innerHTML='&#10148;';tickone.classList.add('tick');tickone.style.color='#2196F3';},1050);"\
      "setTimeout(function(){tickone.classList.remove('tick');},2050);"\
      "}}"\
      "};"\
      "</script>"\
      "</body>"\
      "</html>"\
    );
  });
  server.on("/log", []() {
    server.send(200, "text/plain", logMsg);
  });
  server.on("/reboot", []() {
    server.send(200, "text/plain", "Rebooting fish feeder");
    log("I/system: rebooting upon request from " + server.client().remoteIP().toString());
    delay(1000);
    ESP.restart();
  });
  server.on("/feed", []() {
    server.send(200, "text/plain", "Feeding now!");
    feed();
  });
  server.on("/setfeedtime", setFeedTime);
  server.on("/showfeedtime", showFeedTime);
  server.on("/cancelfeedtime", cancelFeedTime);
  server.begin();

  setupWifi();
  setupTime();

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.begin();
}


void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  time_t now = time(0);
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now);
  int hour = timeinfo->tm_hour;
  int minute = timeinfo->tm_min;

  if (feedTimeData.set == 1 && hour == feedTimeData.hour && minute == feedTimeData.minute) {
    if (!fed) {
      feed();
      fed = true;
    }
  } else {
    fed = false;
  }
}


void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(STA_SSID, STA_PSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  log("I/wifi  : wifi connected. IP address => " + WiFi.localIP().toString());
}


void feed() {
  log("I/feed  : feeding now");
  servo.attach(SERVO_PIN);
  delay(1);
  servo.write(180);
  delay(1500);
  servo.write(0);
  delay(500);
  servo.detach();
  log("I/feed  : done feeding");
}


void log(String msg) {
  time_t now = time(0);
  logTime = ctime(&now);
  logTime.trim();
  logMsg = logMsg + "[" + logTime + "] " + msg + "\n";
  Serial.println(msg);
}


void setupTime() {
  configTime(TIMEZONE * 3600, DST, "pool.ntp.org", "time.nist.gov");
  log("I/time  : waiting for time");
  while (!time(nullptr)) {
    delay(100);
  }
  delay(100);
  time_t now = time(0);
  logTime = ctime(&now);
  logTime.trim();
  log("I/time  : time obtained via NTP => " + logTime);
}


void setFeedTime() {
  log("I/setter: started setFeedTime upon request from " + server.client().remoteIP().toString());
  if (server.arg("hour") == "" || server.arg("minute") == "") {
    server.send(400, "text/plain", "Feed time not specified!");
  } else {
    int feedTimeHour = server.arg("hour").toInt();
    int feedTimeMinute = server.arg("minute").toInt();
    feedTimeData.set = 1;
    feedTimeData.hour = feedTimeHour;
    feedTimeData.minute = feedTimeMinute;
    EEPROM.get(eepromAddr, feedTimeDataOld);
    if (feedTimeDataOld.set == feedTimeData.set && feedTimeDataOld.hour == feedTimeData.hour && feedTimeDataOld.minute == feedTimeData.minute) {
      server.send(200, "text/plain", "Feed time already set for " + String(feedTimeData.hour) + ":" + String(feedTimeData.minute));
    } else {
      EEPROM.put(eepromAddr, feedTimeData);
      EEPROM.commit();
      server.send(200, "text/plain", "Feed time set for " + String(feedTimeData.hour) + ":" + String(feedTimeData.minute));
    }
  }
  log("I/setter: completed setFeedTime");
}


void showFeedTime() {
  log("I/setter: started showFeedTime upon request from " + server.client().remoteIP().toString());
  if (feedTimeData.set == 0) {
    server.send(200, "text/plain", "No feed time set");
  } else {
    EEPROM.get(eepromAddr, feedTimeData);
    server.send(200, "text/plain", "Feed time is set for " + String(feedTimeData.hour) + ":" + String(feedTimeData.minute));
  }
  log("I/setter: completed showFeedTime");
}


void cancelFeedTime() {
  log("I/setter: started cancelFeedTime upon request from " + server.client().remoteIP().toString());
  if (EEPROM.read(eepromAddr) == 0) {
    server.send(200, "text/plain", "No feed time set");
  } else {
    EEPROM.write(0, 0);
    EEPROM.commit();
    feedTimeData.set = 0;
    server.send(200, "text/plain", "Feed time cancelled");
  }
  log("I/setter: completed cancelFeedTime");
}
