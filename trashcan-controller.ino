#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SPI.h>
#include <LoRa.h>
//LCD for show status and trash fill
LiquidCrystal_I2C lcd(0x27, 16, 2);
//Servo for open and close trash cap
Servo myservo;
//Ultra Sonic for check trash capacity
const unsigned int CAP_TRIG_PIN = 8;
const unsigned int CAP_ECHO_PIN = 7;
//Ultra Sonic  for check people nearby
const unsigned int CHECK_TRIG_PIN = 4;
const unsigned int CHECK_ECHO_PIN = 3;

const unsigned int SERVO_PIN = 5;
const unsigned int MIN_DISTANCE = 80;

const unsigned int DELAYS = 300;
const unsigned int BAUD_RATE = 9600;
int pos = 180;
const int numReadings = 10;
String trashCapStatus = "CLOSE";
long capReadings[numReadings];    // the readings from the analog input
int capReadIndex = 0;             // the index of the current reading
long capTotal = 0;                // the running total
long capAverage = 0;              // the average
long checkReadings[numReadings];  // the readings from the analog input
int checkReadIndex = 0;           // the index of the current reading
long checkTotal = 0;              // the running total
long checkAverage = 0;            // the average
long distance = 0;
long currentTrashFill = 0;

void onReceiveDataFromESPControllerV2();
String dataString = "2024-3-9,16:06:01";

void setup() {
  Serial.begin(BAUD_RATE);
  myservo.attach(SERVO_PIN);
  lcd.init();
  lcd.backlight();
  myservo.write(0);
  pinMode(CAP_TRIG_PIN, OUTPUT);
  pinMode(CHECK_TRIG_PIN, OUTPUT);
  pinMode(CAP_ECHO_PIN, INPUT);
  pinMode(CHECK_ECHO_PIN, INPUT);
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    capReadings[thisReading] = 0;
    checkReadings[thisReading] = 0;
  }
  while (!Serial)
    ;
  Serial.println("LoRa Sender");
  if (!LoRa.begin(433E6)) {  // or 915E6, the MHz speed of yout module
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  // Timer1.initialize(500000);
  // Timer1.attachInterrupt(sendDataOnline);
}

void loop() {
  openclose();  
  sendDataLoRa();
  delay(100);
  onReceiveDataFromESPControllerV2();
  screenView();
}

void openclose() {
  long distance = getDistanceFromUltraSonicSensor(2);
  if (distance < 20) {
    myservo.write(90);
    pos = 90;
    trashCapStatus = "OPEN";
  }
  if (distance > 20) {
    myservo.write(180);
    pos = 0;
    trashCapStatus = "CLOSE";
  }
}

long percentTrashFill() {
  long distance = getDistanceFromUltraSonicSensor(1);
  // currentTrashFill = (25 - distance) * 2;
  currentTrashFill = map(distance, 25, 5, 0, 100);
  if (currentTrashFill < 0) currentTrashFill = 0;
  if (currentTrashFill > 100) currentTrashFill = 100;
  return currentTrashFill;
}

void screenView() {
  int commaIndex = dataString.indexOf(',');
  Serial.print(commaIndex);
  String date = dataString.substring(0, commaIndex);
  String time = dataString.substring(commaIndex + 1);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(date + " " + currentTrashFill);
  lcd.setCursor(1, 1);
  lcd.print(time);
  // lcd.print(" " + currentTrashFill + " %");
  delay(100);
}

long getDistanceFromUltraSonicSensor(int distance_device) {
  unsigned long duration = 0;
  switch (distance_device) {
    case 1:
      digitalWrite(CAP_TRIG_PIN, LOW);
      delayMicroseconds(2);
      digitalWrite(CAP_TRIG_PIN, HIGH);
      delayMicroseconds(10);
      digitalWrite(CAP_TRIG_PIN, LOW);
      duration = pulseIn(CAP_ECHO_PIN, HIGH);
      break;
    case 2:
      digitalWrite(CHECK_TRIG_PIN, LOW);
      delayMicroseconds(2);
      digitalWrite(CHECK_TRIG_PIN, HIGH);
      delayMicroseconds(10);
      digitalWrite(CHECK_TRIG_PIN, LOW);
      duration = pulseIn(CHECK_ECHO_PIN, HIGH);
      break;
  }
  return (34 * duration) / 2000;
}

void sendDataLoRa() {
  long temp = percentTrashFill();
  String send = (String)temp + "," + trashCapStatus;

  LoRa.beginPacket();
  LoRa.print(send);
  LoRa.endPacket();

  Serial.println("Data sent via LoRa: " + send);
  LoRa.receive();
  delay(DELAYS);
}

void onReceiveDataFromESPControllerV2() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    dataString = "";
    Serial.print("Received packet: ");
    while (LoRa.available()) {
      char c = LoRa.read();
      dataString += c;
    }
    Serial.println(dataString);
    LoRa.packetRssi();
  }
}
