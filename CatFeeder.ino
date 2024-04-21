#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include "WiFiSecrets.h"
#include "CFTime.h"

#define ASCEND false
#define DESCEND true
#define COIL D1
#define LED D2

const char* ssid = MY_SSID; 
const char* password = MY_KEY;
const char* ntpServer = "ru.pool.ntp.org";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, 10800, 60000);

ESP8266WebServer server(80);

//Default time
CFTime feedTime = CFTime(5,50);

String currentTime = "05:50";
String gpioStatus = "0";
bool coilTime = false;

String ledToggleMessage = "";

void setup() 
{
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize GPIO pins
  pinMode(COIL, OUTPUT);
  pinMode(LED, OUTPUT);

  // Attach routes to the server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/updateTime", HTTP_POST, handleUpdateTime);
  server.on("/toggleLED", HTTP_GET, handleToggleCoil);
  server.on("/getTime", HTTP_GET, handleGetTime);
  server.on("/getGPIOStatus", HTTP_GET, handleGetGPIOStatus);

  // Start server
  server.begin();

  // Initialize NTP
  timeClient.begin();
}

void loop() 
{
  server.handleClient();
  timeClient.update();  
  
  if (feedTime.IsTime(timeClient.getHours(),timeClient.getMinutes())) 
  {
    //Serial.println("Coil Time!");
    if(coilTime == false)
    {
      CoilAction(COIL); 
      coilTime = true;//Block re-opening of the cap
    }
  }
  else
  {
    coilTime = false;
  }
    

  if(timeClient.getMinutes()%2 == 0)//Blink every two minutes
  {
    digitalWrite(LED, HIGH);
  }
  else
  {
    digitalWrite(LED, LOW);
  }
}

void handleRoot() 
{
  String html = "<html><head>";
  html += "<script>";
  html += "function updateValues() {";
  html += "  var xhrTime = new XMLHttpRequest();";
  html += "  xhrTime.onreadystatechange = function() {";
  html += "    if (xhrTime.readyState == 4 && xhrTime.status == 200) {";
  html += "      document.getElementById('currentTime').innerHTML = xhrTime.responseText;";
  html += "    }";
  html += "  };";
  html += "  xhrTime.open('GET', '/getTime', true);";
  html += "  xhrTime.send();";

  html += "  var xhrGPIO = new XMLHttpRequest();";
  html += "  xhrGPIO.onreadystatechange = function() {";
  html += "    if (xhrGPIO.readyState == 4 && xhrGPIO.status == 200) {";
  html += "      document.getElementById('gpioStatus').innerHTML = 'Открытие замка: ' + xhrGPIO.responseText;";
  html += "    }";
  html += "  };";
  html += "  xhrGPIO.open('GET', '/getGPIOStatus', true);";
  html += "  xhrGPIO.send();";
  html += "}";
  html += "setInterval(updateValues, 1000);"; // Update every 1 second
  html += "</script>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'>";
  html += "<title>КотоКормушка</title>\n";
  html +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  html +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  /*html +=".button {display: block;width: 100px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  html +=".button-on {background-color: #1abc9c;}\n";
  html +=".button-on:active {background-color: #16a085;}\n";
  html +=".button-off {background-color: #34495e;}\n";
  html +=".button-off:active {background-color: #2c3e50;}\n";
  html +="input {width: 100px;padding: 13px 30px;font-size: 25px;margin: 0px auto 35px;}\n";*/
  //html += ".button{width: 100px;}
  html +="p {font-size: 16px;color: #888;margin-bottom: 10px;}\n";   
  html +="</style>\n";
  html += "</head><body onload='updateValues()'>";
  html += "<h1>Кормушка для Марсика</h1>";
  html += "<h3>С управлением по Wi-Fi и таймером</h3>";

  // Display time
  html += "<p id='currentTime'>Текущее время: " + timeClient.getFormattedTime() + "</p>";

  // Display GPIO pin status
  html += "<p id='gpioStatus'>Открытие замка: " + gpioStatus + "</p>";

  // Display LED toggle message
  if (ledToggleMessage.length() > 0) {
    html += "<p id='ledToggleMessage'>" + ledToggleMessage + "</p>";
    ledToggleMessage = ""; // Reset message
  }

  // Button to toggle GPIO D1
  html += "<form id='toggleLEDForm' action='/toggleLED' method='get'>";
  html += "<input type='button' value='Открыть' onclick='toggleLEDAndReturn()' />";
  html += "</form>";

  // Form to set time
  html += "<form id='timeForm' action='/updateTime' method='post'>";
  html += "<label>Время открытия:</label>";
  html += "<input type='time' name='setTime' id='setTime' value='" + currentTime + "' />";
  html += "<input type='button' value='ОК' onclick='updateTimeAndReturn()' />";
  html += "</form>";

  // JavaScript to restore form values and return to initial state
  html += "<script>";
  html += "function updateTimeAndReturn() {";
  html += "  var form = document.getElementById('timeForm');";
  html += "  var formData = new FormData(form);";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('POST', '/updateTime', true);";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status === 200) {";
  html += "      updateValues();";
  html += "    }";
  html += "  };";
  html += "  xhr.send(formData);";
  html += "}";
  html += "function toggleLEDAndReturn() {";
  html += "  var form = document.getElementById('toggleLEDForm');";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/toggleLED', true);";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status === 200) {";
  html += "      ledToggleMessage = xhr.responseText;";
  html += "      updateValues();";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "</script>";

  html += "</body></html>";

  server.send(200, "text/html; charset=utf-8", html);
}

void handleUpdateTime() //Seting of the opening time from the Web page
{
  currentTime = server.arg("setTime");
  feedTime.SetTime(parseHours(currentTime),parseMinutes(currentTime));  
  server.send(200, "text/plain", "Time updated successfully");
}

void handleToggleCoil() //Handle open of the cap
{
  CoilAction(COIL);
  server.send(200, "text/plain", "LED toggled");
}

void handleGetTime() 
{
  server.send(200, "text/plain", timeClient.getFormattedTime());
}

void handleGetGPIOStatus() 
{
  server.send(200, "text/plain", gpioStatus);
}

void CoilAction(int pin)
{
  int pwmState = 0;  
  while(pwmState < 1024) //Soft starting coil during 1.024 sec
  {
    analogWrite(COIL,pwmState);
    pwmState+=2;
    delay(2);
  }
  delay(500); //Waiting for a half second
  while(pwmState > 0) //Soft stopping coil during 2.048 sec
  {
    analogWrite(COIL,pwmState);
    pwmState-=2;
    delay(2);
  }  
}

int parseHours(String timeString)
{
  String result = timeString.substring(0,timeString.indexOf(":"));
  //Serial.println(result);
  return result.toInt();
}

int parseMinutes(String timeString)
{
  String result = timeString.substring(timeString.indexOf(":")+1,5);
  //Serial.println(result);
  return result.toInt();
}
