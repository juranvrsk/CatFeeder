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
//NTPClient timeClient(ntpUDP, ntpServer, 10800, 60000);
NTPClient* timeClient;

ESP8266WebServer server(80);

//Default time
CFTime feedTime = CFTime(5,50);
String currentTime = "05:50";
String gpioStatus = "0";
bool coilTime = false;

String coilToggleMessage = "";

void setup() 
{
  Serial.begin(115200);
  Serial.println("Start ESP8266");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  timeClient = new NTPClient(ntpUDP, ntpServer, 10800, 60000);

  // Initialize GPIO pins
  pinMode(COIL, OUTPUT);
  pinMode(LED, OUTPUT);

  // Attach routes to the server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/updateTime", HTTP_POST, handleUpdateTime);
  server.on("/toggleCoil", HTTP_GET, handleToggleCoil);
  server.on("/getTime", HTTP_GET, handleGetTime);
  server.on("/getGPIOStatus", HTTP_GET, handleGetGPIOStatus);

  // Start server
  server.begin();

  // Initialize NTP
  timeClient->begin();
}

void loop() 
{
  server.handleClient();
  timeClient->update();  
  
  if (feedTime.IsTime(timeClient->getHours(),timeClient->getMinutes())) 
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
    

  if(timeClient->getMinutes()%2 == 0)//Blink every two minutes
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
  html +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;font-size: 48px;} h3 {color: #444444;margin-bottom: 50px;font-size: 32px;}\n";
  html +="p {font-size: 32px;color: #888;margin-bottom: 10px;} input[type=button]{padding: 16px 128px;font-size: 32;} input[type=time]{padding: 16px 128px;font-size:32;} label{font-size:32;}\n";   
  html +="</style>\n";
  html += "</head><body onload='updateValues()'>";
  html += "<h1>The cat feeder</h1>";
  html += "<h3>With the Wi-Fi and timer</h3>";

  // Display time
  html += "<p id='currentTime'>Current: " + timeClient->getFormattedTime() + "</p>";

  // Display GPIO pin status
  html += "<p id='gpioStatus'>Cap open: " + gpioStatus + "</p>";

  // Display coil toggle message
  if (coilToggleMessage.length() > 0) {
    html += "<p id='coilToggleMessage'>" + coilToggleMessage + "</p>";
    coilToggleMessage = ""; // Reset message
  }

  // Button to toggle GPIO D1 (coil)
  html += "<form id='toggleCoilForm' action='/toggleCoil' method='get'>";
  html += "<input type='button' value='Open' onclick='toggleCoilAndReturn()' />";
  html += "</form>";

  // Form to set time
  html += "<form id='timeForm' action='/updateTime' method='post'>";
  html += "<label>Open Time:</label>";
  html += "<br>";
  html += "<input type='time' name='setTime' id='setTime' value='" + currentTime + "' />";
  html += "<br>";
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
  html += "function toggleCoilAndReturn() {";
  html += "  var form = document.getElementById('toggleCoilForm');";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/toggleCoil', true);";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status === 200) {";
  html += "      coilToggleMessage = xhr.responseText;";
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
  server.send(200, "text/plain", "Coil toggled");
}

void handleGetTime() 
{
  server.send(200, "text/plain", timeClient->getFormattedTime());
}

void handleGetGPIOStatus() 
{
  server.send(200, "text/plain", gpioStatus);
}

void CoilAction(int pin)//Soft powering up and down
{
  int pwmState = 0;  
  while(pwmState < 1024) //Soft powering up coil during 1.024 sec
  {
    analogWrite(COIL,pwmState);
    pwmState+=2;
    delay(2);
  }
  delay(500); //Waiting for a half second
  while(pwmState > 0) //Soft powering down coil during 2.048 sec
  {
    analogWrite(COIL,pwmState);
    pwmState-=2;
    delay(2);
  }  
}

int parseHours(String timeString)
{
  String result = timeString.substring(0,timeString.indexOf(":"));
  return result.toInt();
}

int parseMinutes(String timeString)
{
  String result = timeString.substring(timeString.indexOf(":")+1,5);
  return result.toInt();
}
