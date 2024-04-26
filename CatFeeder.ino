#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include "WiFiSecrets.h"
#include "CFTime.h"

#define COIL D1
#define LED D2

const char* ssid = MY_SSID; 
const char* password = MY_KEY;
//const char* ntpServer = "ru.pool.ntp.org";
const char* ntpServer = "time.nist.gov";

WiFiUDP ntpUDP;
NTPClient* timeClient;

ESP8266WebServer server(80);

//Default time
CFTime::TimeStamp feedTime = {5,50,0};
CFTime::TimeStamp updatePeriod = {1,0,0}; //Every hour
CFTime::TimeStamp ledPeriod = {0,2,0}; //Every two minutes
CFTime RTC;
String currentTime = "05:50";
String gpioStatus = "0";
bool coilTime = false;
unsigned long lastMillis = 0;

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
  delay(3000); //Small pause and update, just in case
  timeClient->update();
  RTC = CFTime(timeClient->getHours(),timeClient->getMinutes(),timeClient->getSeconds());
}

void loop() 
{
  RTC.Tick();
  server.handleClient();
  if(RTC.Period(updatePeriod))
  {
    timeClient->update();
  }  
  if (RTC.IsTime(feedTime))
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
  if(RTC.Period(ledPeriod))//Blink 
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
  String html = "<html>\n<head>";
  html += "<script>";
  html += "function updateValues() {";
  html += "  fetch('/getTime')";
  html += "     .then(response => response.text())";
  html += "     .then(text => document.getElementById('currentTime').innerHTML = text);";
  html += "  fetch('/getGPIOStatus')";
  html += "     .then(response => response.text())";
  html += "     .then(text => document.getElementById('gpioStatus').innerHTML = 'Cap open:'+ text);";
  html += "}";
  html += "function updateTimeAndReturn() {";
  html += "    const formData = new FormData(document.getElementById('timeForm'));";
  html += "    fetch('/updateTime', { method: 'POST', body: formData })";
  html += "       .then(response => response.ok && updateValues());";
  html += "}";
  html += "function toggleCoilAndReturn() {";
  html += "    fetch('/toggleCoil')";
  html += "       .then(response => response.ok && updateValues());";
  html += "}";
  html += "setInterval(updateValues, 1000);"; // Update every 1 second
  html += "</script>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'>";
  html += "<title>The cat Feeder</title>\n";
  html +="<style>";
  html +="  body {font-family: Helvetica; margin: 50px auto; text-align: center;}";
  html +="  h1 {color: #444444; margin: 50px auto 30px; font-size: 48px; }";
  html +="  h3 {color: #444444; margin-bottom: 50px; font-size: 32px; }";
  html +="  p {font-size: 32px; color: #888; margin-bottom: 10px; }";
  html +="  form {margin-bottom: 20px;}";
  html +="  input[type='time'], button[type='button'] {width: 100%; padding: 16px; font-size: 32px; margin-bottom: 10px; text-align:center;}";
  html +="  label {font-size: 32px; margin-bottom: 10px; }";  
  html +="</style>\n";
  html += "</head>\n<body>";
  html += "<h1>The cat feeder</h1>";
  html += "<h3>With the Wi-Fi and timer</h3>";
  // Display time
  html += "<p id='currentTime'>Current: " + RTC.ToString() + "</p>";
  // Display GPIO pin status
  html += "<p id='gpioStatus'>Cap open: " + gpioStatus + "</p>";
  // Display coil toggle message
  if (coilToggleMessage.length() > 0) {
    html += "<p id='coilToggleMessage'>" + coilToggleMessage + "</p>";
    coilToggleMessage = ""; // Reset message
  }

  // Button to toggle GPIO D1 (coil)
  html += "<form id='toggleCoilForm'>";
  html += "<button type='button' onclick='toggleCoilAndReturn()'>Open</button>";
  html += "</form>";

  // Form to set time
  html += "<form id='timeForm'>";
  html += "<label for='setTime'>Open Time:</label>";
  html += "<input type='time' name='setTime' id='setTime' value='" + currentTime + "' />";
  html += "<button type='button' onclick='updateTimeAndReturn()'>OK</button>";
  html += "</form>";
  html += "</body></html>";

  server.send(200, "text/html; charset=utf-8", html);

  server.send(200, "text/html; charset=utf-8", html);
}

void handleUpdateTime() //Seting of the opening time from the Web page
{
  currentTime = server.arg("setTime");
  feedTime.Hour = parseHours(currentTime);
  feedTime.Minute = parseMinutes(currentTime);  
  server.send(200, "text/plain", "Time updated successfully");
}

void handleToggleCoil() //Handle open of the cap
{
  CoilAction(COIL);
  server.send(200, "text/plain", "Coil toggled");
}

void handleGetTime() 
{
  server.send(200, "text/plain", RTC.ToString());
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

//Soft powering up and down. It haves the three stages: voltage increasing, pause and voltage decreasing
//The speed is the time step, higher is slower. Period of work depends on it
/*void CoilAction(int pin, int speed)
{
    int pwmState = 0;
    int cycleCounter = 0;//Three stages 0-1023 is power up,1024-2047 is pause,2048-3071 is power down
    unsigned long nowMillis = millis();
    if(nowMillis-lastMillis == speed)
    {
      if(cycleCounter < 1024)
      {        
        pwmState+=2;
      }
      else if(cycleCounter >=2048)
      {
        pwmState-=2;
      }

      cycleCounter++;
    }
}*/

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
