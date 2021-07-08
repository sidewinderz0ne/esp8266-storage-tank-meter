#include <Arduino.h>
#include <Wire.h>

//Lidar
#include <LIDARLite.h>

//Temp
#include <OneWire.h>
#include <DallasTemperature.h>

//WEB
#include <ESP8266WiFi.h>
#include <Hash.h>
/* #include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h> */

#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = 2;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

LIDARLite myLidarLite;

String temperature = "";

// Replace with your network credentials
//const char* ssid = "XLGO-8L07";
//const char* password = "agrotech20";
const char *ssid = "gorengan";
const char *password = "gorengan2021";

// Create AsyncWebServer object on port 80
/* AsyncWebServer server(80); */

String readDSTemperature();
String readTemp();
float readDist();
void postData(String header);
String processor(const String &var);

String readDSTemperature()
{
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == -127.00)
  {
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  }
  else
  {
    Serial.print("Temperature Celsius: ");
    Serial.println(tempC);
  }
  return String(tempC);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ds-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP Lidar + DS18B20</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-ruler-vertical" style="color:#800000;"></i> 
    <span class="ds-labels">Space</span>
    <span id="jarak">%JARAK%</span>
    <sup class="units">CM</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 1000) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("jarak").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/jarak", true);
  xhttp.send();
}, 1000) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DS18B20 values
/* String processor(const String &var)
{
  //Serial.println(var);
  if (var == "TEMPERATURE")
  {
    return temperature;
  }
  else if (var == "JARAK")
  {
    return jarak;
  }
  return String();
} */

void setup()
{
  Serial.begin(115200);
  myLidarLite.begin(0, true);
  myLidarLite.configure(0);
  delay(1000);
  sensors.begin();
  temperature = readDSTemperature();

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  /*   // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", temperature.c_str()); });
  server.on("/jarak", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", jarak.c_str()); });
  // Start server
  server.begin(); */
}

void loop()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_OFF);
    delay(60000);
    WiFi.begin(ssid, password);
    delay(60000);
  }
  float jarakR = readDist();
  temperature = readTemp();
  String post = "temp=" + temperature + "&jarak=" + jarakR + "&lokasi=" + "1";
  postData(post);
  delay(60000);
}

float readDist()
{
  String jarak = "";
  float jarakR = 0.0;
  Serial.println(myLidarLite.distance());
  int i = 30;
  int a = 0;
  while (i > 0)
  {
    jarak = myLidarLite.distance();
    Serial.println(jarak);
    /* jarak = (rand() % 100) + 1; */
    if (jarak == "nack")
    {
      jarak = "0";
      //alert telegram
    }
    float jF = jarak.toFloat();
    int failed = 0;
    //cek besok
    while (jF > 1900.0 || jF < 550.0)
    {
      //http://wg-monitoring.srs-ssms.com/config/pstdata.php?id=1
      jarak = myLidarLite.distance();
      Serial.println(jarak);
      jF = jarak.toFloat();
      delay(1000);
      if (failed == 300)
      {
        postData("id=0");
      }
      failed++;
    }

    float j = jarakR + jF;

    if (a == 1)
    {
      jarakR = j / 2;
    }
    else
    {
      jarakR = j;
    }
    /*     Serial.print("jF: ");
    Serial.println(jF);
    Serial.print("j: ");
    Serial.println(j);
    Serial.print("jarak: ");
    Serial.println(jarak);
    Serial.print("jarakR: ");
    Serial.println(jarakR); */
    delay(1000);
    a = 1;
    i--;
  }
  return jarakR;
}

String readTemp()
{
  String temp;
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");
  temp = readDSTemperature();
  int failed = 0;
  while (temp == "--")
  {
    sensors.requestTemperatures();
    temperatureC = sensors.getTempCByIndex(0);
    Serial.print(temperatureC);
    Serial.println("ºC");
    temp = readDSTemperature();
    delay(1000);
    if (failed == 300)
    {
      postData("id=0");
    }
    failed++;
  }
  return temp;
}

void postData(String header)
{
  //Post Data
  String postData;
  postData = header;

  /*   HTTPClient http; //Declare object of class HTTPClient

  http.begin("https://wg-monitoring.srs-ssms.com/config/pstdata.php?"+postData); //Specify request destination
  http.addHeader("Content-Type", "application/x-www-form-urlencoded"); //Specify content-type header

  int httpCode = http.GET(); //Send the request
  int httpCode = http.POST(postData); //Send the request
  String payload = http.getString();  //Get the response payload

  Serial.print("isi: ");
  Serial.println(postData);
  Serial.print("http code: ");
  Serial.println(httpCode); //Print HTTP return code
  Serial.print("payload: ");
  Serial.println(payload); //Print request response payload

  http.end(); //Close connection */

  WiFiClient client;

  HTTPClient http;

  Serial.print("[HTTP] begin...\n");
  if (http.begin(client, "http://wg-monitoring.srs-ssms.com/config/pstdata.php?" + postData))
  { // HTTP

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        String payload = http.getString();
        Serial.println(payload);
      }
    }
    else
    {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
  else
  {
    Serial.printf("[HTTP} Unable to connect\n");
  }
}
