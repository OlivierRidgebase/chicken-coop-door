#include <ESP8266WiFiMulti.h> 
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = -4*3600;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
unsigned long time_now;
long previousMillisSecond = 0;
long previousMillisSync = 0; 
long previousMillisOpenCloseTime = 0;
long addSecond = 1000; 
long syncTime = 60000; 
long checkOpenCloseTimeFreq = 15000; 
int close_sec = 35;
int open_sec = 35;
String open_time = "09:00";
String close_time = "10:00";
ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
WiFiClient client;
HTTPClient http;

void handleRoot();              // function prototypes for HTTP handlers
void handleLED();
void handleNotFound();

// Assign output variables to GPIO pins
int ENA = 4; //4;
int IN1 = 0; //0;
int IN2 = 2; //2;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
    
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  wifiMulti.addAP("EMOI_LGM_EXT", "MyPassword");   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("EMOI_LGM", "MyPassword");   // add Wi-Fi networks you want to connect to
  Serial.println("Connecting ...");
 
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());      
  Serial.println();   Serial.println("Start");   Serial.println();

  timeClient.begin();
  time_now = getTimeFronNTP();
  getOpenCloseTime();
  server.on("/", HTTP_GET, handleRoot);     // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/OPEN", HTTP_POST, openDoor);  // Call the 'handleLED' function when a POST request is made to URI "/LED"
  server.on("/CLOSE", HTTP_POST, closeDoor);  // Call the 'handleLED' function when a POST request is made to URI "/LED"
  server.on("/CHANGETIME", HTTP_GET, changeTime);  
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
   server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
}

unsigned long getTimeFronNTP() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

void loop() {
  String heure = timeNow();
  unsigned long currentMillisOpenCloseTime = millis();
  if(currentMillisOpenCloseTime - previousMillisOpenCloseTime > checkOpenCloseTimeFreq) {
    previousMillisOpenCloseTime = currentMillisOpenCloseTime; 
    getOpenCloseTime();
  }
  if(heure == open_time+":00"){
    openDoor();
  }
  if(heure == close_time+":00"){
    closeDoor();
  }
  server.handleClient();                    // Listen for HTTP requests from clients
}

void handleRoot() {                         // When URI / is requested, send a web page with a button to toggle the LED
  server.send(200, "text/html", "<style>div{ font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}.button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}.button2 {background-color: #77878A;}</style><div><h2>Porte du Poulailler</h2><form action=\"/OPEN\" method=\"POST\"><input type=\"submit\"  class=\"button\" value=\"Ouvrir Porte\"></form><form action=\"/CLOSE\" method=\"POST\"><input type=\"submit\"  class=\"button button2\" value=\"Fermer Porte\"></form><br><br><hr><br><form action=\"/CHANGETIME\" method=\"GET\"><input type=\"text\" value=\""+open_time+"\" name=\"ot\"><br><input type=\"text\" value=\""+close_time+"\" name=\"ct\"><br><input type=\"submit\"  class=\"button button2\" value=\"Change Heure\"></div>");
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

String timeNow(){
 unsigned long currentMillisSecond = millis();
 unsigned long currentMillisSync = millis();
 if(currentMillisSecond - previousMillisSecond > addSecond) {
  previousMillisSecond = currentMillisSecond; 
  time_now = time_now + 1;
  setTime(time_now);
 }
 if(currentMillisSync - previousMillisSync > syncTime) {
  previousMillisSync = currentMillisSync; 
  Serial.println("Synchronize time");
  time_now = getTimeFronNTP(); 
  setTime(time_now);  
 }
 char formated_time[8];
 sprintf(formated_time, "%02d:%02d:%02d", hour(), minute(), second());
 return String(formated_time);
}

bool checkInternet(){
  int httpResponseCode;
  bool response = false;
  // Your IP address with path or Domain name with URL path 
  http.begin(client, "http://safe-plains-95452.herokuapp.com/coop/");
  // Send HTTP POST request
  httpResponseCode = http.GET();
  if(http.GET() == 200){
   response = true;
  }
  // Free resources
  http.end();
  return response;
}
String httpGETRequest(const String serverName) {

  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode==200) {
    payload = http.getString();
  }else{
    Serial.println("Reboot ESP");
    ESP.restart();
  }
  // Free resources
  http.end();

  return payload;
}

void getOpenCloseTime(){
  String urlResponse;
  DynamicJsonDocument doc(1024);
  urlResponse = httpGETRequest("http://safe-plains-95452.herokuapp.com/coop/");
  Serial.println(urlResponse);
  deserializeJson(doc, urlResponse);
  JsonObject obj = doc.as<JsonObject>();
  open_time = obj["open_time"].as<String>();
  close_time = obj["close_time"].as<String>();
}
void openDoor(){
  Serial.print("Open DOOR: ");
  digitalWrite(ENA, HIGH);  
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  delay(close_sec * 1000); // now turn off motors
  Serial.print("END OPEN ");
  digitalWrite(ENA, LOW);  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303); 
}


void closeDoor(){
  Serial.print("CLOSE DOOR: ");
  digitalWrite(ENA, HIGH);  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  delay(open_sec * 1000); // now turn off motors
  Serial.print("END CLOSE ");
  digitalWrite(ENA, LOW);  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303); 
}

void changeTime(){
  open_time = server.arg("ot");
  close_time = server.arg("ct");
  setOpenCloseTime();
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303); 
}

void setOpenCloseTime(){
  httpGETRequest("http://safe-plains-95452.herokuapp.com/change_time/?open_time="+open_time+"&close_time="+close_time);
}
