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
uint eepromAddr[6] = {0, 50, 100, 150, 200, 250};
int numOfAddr = sizeof(eepromAddr) / sizeof(eepromAddr[0]);
bool opened = false;
ESP8266WebServer server(80);
typedef struct {
  uint set = 0;
  uint hour = 0;
  uint minute = 0;
} eepromData;
eepromData feedTimeData[6] = {};
eepromData feedTimeDataOld[6] = {};

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
  for (int i = 0; i < numOfAddr; i++) {
    EEPROM.get(eepromAddr[i], feedTimeData[i]);
  }

  server.on("/", []() {
    server.send(200, "text/html", \
      "<a href=\"/app\">/app</a><br>"\
      "<a href=\"/log\">/log</a><br>"\
      "<a href=\"/reboot\">/reboot</a><br>"\
      "<a href=\"/feed\">/feed</a><br>"\
      "<a href=\"/setfeedtime?cycle=0&hour=14&minute=44\">/setfeedtime?cycle=0&hour=14&minute=44</a><br>"\
      "<a href=\"/showfeedtime?cycle=0\">/showfeedtime?cycle=0</a><br>"\
      "<a href=\"/cancelfeedtime?cycle=0\">/cancelfeedtime?cycle=0</a><br>"\
      "<br><p><small>"\
      "Powered by: <a href=\"https://github.com/ameer1234567890/FishFeeder\">FishFeeder</a> | "\
      "Chip ID: " + String(ESP.getChipId()) + \
      "</small></p>"\
    );
  });
  server.on("/app", []() {
    String slotsHtml;
    String setState[numOfAddr];
    String timeState[numOfAddr];
    for (int i = 0; i < numOfAddr; i++) {
      if (feedTimeData[i].set == 0) {
        timeState[i] = "disabled";
      } else {
        setState[i] = "checked";
      }
      slotsHtml = slotsHtml + \
      "<label class=\"slotname\">SLOT " + i + ":</label>"\
      "<span class=\"spacer\"></span>"\
      "<label class=\"switch\">"\
      "<input type=\"checkbox\" name=\"set" + String(i) + "\" id=\"set" + i + "\" " + setState[i] + ">"\
      "<span class=\"slider round\"></span>"\
      "</label>"\
      "<span class=\"spacer\"></span>"\
      "<input type=\"time\" id=\"time" + i + "\" name=\"time" + i + "\" "\
      "value=\"" + feedTimeData[i].hour + ":" + feedTimeData[i].minute + "\" class=\"timeselector\" " + timeState[i] + ">"\
      "<span class=\"spacer\"></span>"\
      "<span class=\"tickmark\" id=\"tick" + i + "\">&#10148;</span>"\
      "<br><br>"\
      "<script>"\
      "set" + i + "=document.querySelector('#set" + i + "');"\
      "time" + i + "=document.querySelector('#time" + i + "');"\
      "tick" + i + "=document.querySelector('#tick" + i + "');"\
      "set" + i + ".onchange=function(e){"\
      "httpRequest=new XMLHttpRequest();"\
      "if(set" + i + ".checked){time" + i + ".disabled='';"\
      "httpRequest.open('GET','/setfeedtime?cycle=" + i + "&hour='+time" + i + ".value.substr(0,2)+'&minute='+time" + i + ".value.substr(3,4));"\
      "httpRequest.send();"\
      "httpRequest.onreadystatechange=function(){"\
      "if(httpRequest.readyState==4){if(httpRequest.status!=200){set" + i + ".checked=false;time" + i + ".disabled='disabled';}"\
      "}}"\
      "}else{time" + i + ".disabled='disabled';httpRequest.open('GET','/cancelfeedtime?cycle=" + i + "');httpRequest.send();"\
      "httpRequest.onreadystatechange=function(){"\
      "if(httpRequest.readyState==4){if(httpRequest.status!=200){set" + i + ".checked=true;time" + i + ".disabled='';}"\
      "}}"\
      "}};"\
      "time" + i + ".oninput=function(e){"\
      "tick" + i + ".style.visibility='visible';tick" + i + ".classList.add('tick');setTimeout(function(){tick" + i + ".classList.remove('tick');},1000);"\
      "};"\
      "tick" + i + ".onclick=function(e){"\
      "httpRequest=new XMLHttpRequest();"\
      "tick" + i + ".style.color='#ccc';"\
      "httpRequest.open('GET','/setfeedtime?cycle=" + i + "&hour='+time" + i + ".value.substr(0,2)+'&minute='+time" + i + ".value.substr(3,4));"\
      "httpRequest.send();"\
      "httpRequest.onreadystatechange=function(){"\
      "if(httpRequest.readyState==4){if(httpRequest.status==200){"\
      "tick" + i + ".innerHTML='&#10004;';tick" + i + ".classList.add('tick');tick" + i + ".style.color='#34995f';"\
      "setTimeout(function(){tick" + i + ".innerHTML='&#10148;';tick" + i + ".classList.remove('tick');tick" + i + ".style.color='#2196F3';tick" + i + ".style.visibility='hidden';},1000);"\
      "}else{"\
      "tick" + i + ".classList.add('tick');"\
      "tick" + i + ".innerHTML='&#x26A0;';tick" + i + ".style.color='#F5BD1F';}"\
      "setTimeout(function(){tick" + i + ".innerHTML='';tick" + i + ".classList.remove('tick');},1000);"\
      "setTimeout(function(){tick" + i + ".innerHTML='&#10148;';tick" + i + ".classList.add('tick');tick" + i + ".style.color='#2196F3';},1050);"\
      "setTimeout(function(){tick" + i + ".classList.remove('tick');},2050);"\
      "}}"\
      "};"\
      "</script>"\
      ;
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
      + slotsHtml + \
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

  for (int i = 0; i < numOfAddr; i++) {
    if (feedTimeData[i].set == 1 && hour == feedTimeData[i].hour && minute == feedTimeData[i].minute) {
      if (!fed) {
        feed();
        fed = true;
      }
    } else {
      fed = false;
    }
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
  if (server.arg("cycle") == "" || server.arg("hour") == "" || server.arg("minute") == "") {
    server.send(400, "text/plain", "Feed time or cycle not specified!");
  } else {
    int feedCycle = server.arg("cycle").toInt();
    int feedTimeHour = server.arg("hour").toInt();
    int feedTimeMinute = server.arg("minute").toInt();
    feedTimeData[feedCycle].set = 1;
    feedTimeData[feedCycle].hour = feedTimeHour;
    feedTimeData[feedCycle].minute = feedTimeMinute;
    EEPROM.get(eepromAddr[feedCycle], feedTimeDataOld[feedCycle]);
    if (feedTimeDataOld[feedCycle].set == feedTimeData[feedCycle].set && feedTimeDataOld[feedCycle].hour == feedTimeData[feedCycle].hour && feedTimeDataOld[feedCycle].minute == feedTimeData[feedCycle].minute) {
      server.send(200, "text/plain", "Feed time already set to " + String(feedTimeData[feedCycle].hour) + ":" + String(feedTimeData[feedCycle].minute) + " for cycle " + feedCycle);
    } else {
      EEPROM.put(eepromAddr[feedCycle], feedTimeData[feedCycle]);
      EEPROM.commit();
      server.send(200, "text/plain", "Feed time set to " + String(feedTimeData[feedCycle].hour) + ":" + String(feedTimeData[feedCycle].minute) + " for cycle " + feedCycle);
      log("I/setter: Feed cycle " + String(feedCycle) + " set to " + String(feedTimeData[feedCycle].hour) + ":" + String(feedTimeData[feedCycle].minute) + " upon request from " + server.client().remoteIP().toString());
    }
  }
}


void showFeedTime() {
  if (server.arg("cycle") == "") {
    server.send(400, "text/plain", "Feed cycle not specified!");
  } else {
    int feedCycle = server.arg("cycle").toInt();
    if (feedTimeData[feedCycle].set == 0) {
      server.send(200, "text/plain", "No feed time set");
    } else {
      EEPROM.get(eepromAddr[feedCycle], feedTimeData[feedCycle]);
      server.send(200, "text/plain", "Feed time is set to " + String(feedTimeData[feedCycle].hour) + ":" + String(feedTimeData[feedCycle].minute) + " for cycle " + feedCycle);
    }
  }
}


void cancelFeedTime() {
  if (server.arg("cycle") == "") {
    server.send(400, "text/plain", "Feed cycle not specified!");
  } else {
    int feedCycle = server.arg("cycle").toInt();
    if (EEPROM.read(eepromAddr[feedCycle]) == 0) {
      server.send(200, "text/plain", "No feed time set");
    } else {
      EEPROM.write(eepromAddr[feedCycle], 0);
      EEPROM.commit();
      feedTimeData[feedCycle].set = 0;
      server.send(200, "text/plain", "Feed time cancelled for cycle " + feedCycle);
      log("I/setter: Feed cycle " + String(feedCycle) + " cancelled upon request from " + server.client().remoteIP().toString());
    }
  }
}
