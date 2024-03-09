#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define ss 15 //ss port = D08
#define rst 16 //reset port = D00 In ESP
#define dio0 4 //call back port only use if use callback
ESP8266WebServer server(8080);
const char* ssid = "AKA Coffee _PL";
const char* password = "123456789";
String dataString = "0,CLOSE";
String timeResult = ",";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
void handle_OnConnect();
void onReceiveDataFromArduinoController(int packetSize);
void handleSendingDateToArduino();
void syncTimeFromTimeServer();
void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  IPAddress myIp = WiFi.softAPIP();
  server.on("/", handle_OnConnect);
  server.on("/adddata", handle_OnGetDataRequest);
  server.begin();
  Serial.print("Starting the web server in local ip: ");
  Serial.print(WiFi.localIP());
  Serial.println("LoRa Receiver");

  while (!Serial);
  LoRa.setPins(ss, rst, dio0);
    if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    delay(100);
    while (1);
  }
  timeClient.begin();
  timeClient.setTimeOffset(25200);
  Serial.println("LoRa initialization successful.");

  LoRa.onReceive(onReceiveDataFromArduinoController);
  LoRa.receive();
}

void loop() {
  delay(2000);
  handleSendingDateToArduino();
  server.handleClient();
  delay(100);
}


void handle_OnGetDataRequest() {
  server.send(200, "text/plane", dataString);
}

const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta http-equiv="Content-Type" content="text/html" viewport="1" />
    <title>Trash-Info</title>
  </head>
  <style type="text/css">
    .button {
      background-color: #4caf50; /* Green */
      border: none;
      color: white;
      padding: 15px 32px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
    }
    body {
      background-color: aquamarine;
    }
  </style>
  <body>
    <div
      style="display: flex; flex-direction: column; align-items: center"
      id="root"
    >
      <h1>TRASH-FILL ESP8266 WEBSERVER</h1>
      <img
        src="https://gifdb.com/images/high/pop-cat-meme-8c6jydnj5lnoopl2.webp"
        style="width: 300px" id="catpop"
      />
      <br />
      <div
        style="
          background-color: aqua;
          text-align: left;
          width: 500px;
          border-radius: 10px;
          border: 1px solid blue;
        "
      >
        <h2>Trashcan-Status: <span id="status">CLOSE</span><br /></h2>
        <h2>Trashcan-Fill: <span id="fill">0</span>%<br /></h2>
      </div>
    </div>
    <script>
      function getData() {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function () {
          if (this.readyState == 4 && this.status == 200) {
            const dataArray = this.responseText.split(",");
            if (dataArray.length != 0) {
              document.getElementById("fill").innerHTML = dataArray[0];
              document.getElementById("status").innerHTML = dataArray[1];
              if(dataArray[1] == "CLOSE") document.getElementById("catpop").src = "https://i.kym-cdn.com/photos/images/original/001/931/955/181.jpg";
              else document.getElementById("catpop").src = "https://i.kym-cdn.com/photos/images/original/001/931/957/b02.jpg";
            }
          }
        };
        xhttp.open("GET", "adddata", true);
        xhttp.send();
      }
      setInterval(function () {
        getData();
      }, 1000);
    </script>
  </body>
</html>
)=====";

void handle_OnConnect() {
  server.send(200, "text/html", webpage); 
}

void onReceiveDataFromArduinoController(int packetSize){
  Serial.print(packetSize);
  if (packetSize) {
    dataString = "";
    Serial.print("Received packet: ");

    // Read packet and store in the global string variable
    while (LoRa.available()) {
      char c = LoRa.read();
      dataString += c;
    }

    // Print the received message
    Serial.println(dataString);
  }
}

void handleSendingDateToArduino(){
  syncTimeFromTimeServer();
  String send = timeResult;

  LoRa.beginPacket();
  LoRa.print(send);
  LoRa.endPacket();

  Serial.println("Data sent via LoRa: " + send);
  LoRa.receive();
  delay(1000);
}

void syncTimeFromTimeServer(){
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  String formattedTime = timeClient.getFormattedTime();
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime);  
  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  int monthDay = ptm->tm_mday;
  Serial.print("Month day: ");
  Serial.println(monthDay);

  int currentMonth = ptm->tm_mon+1;
  Serial.print("Month: ");
  Serial.println(currentMonth);

  int currentYear = ptm->tm_year+1900;
  Serial.print("Year: ");
  Serial.println(currentYear);

  //Print complete date:
  String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  Serial.print("Current date: ");
  Serial.println(currentDate);

  timeResult = currentDate + "," + formattedTime;
}