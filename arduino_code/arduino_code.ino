
#include <PubSubClient.h>
#include <WiFi.h>
#include "DHTesp.h"
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

#define LED_BUILTIN 2
#define LDR_PIN 32
#define SERVO_PIN 18
#define BUZZER_PIN 33
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     0
#define UTC_OFFSET_DST 0
const int DHT_PIN = 15;

Servo servoMotor;
DHTesp dhtSensor;
WiFiClient espclient;
PubSubClient mqttClient(espclient);
LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 20, 4);
char tempAr[6];
char humiAr[6];
char buzDelay[6];
char buzFreq[6];
char buzMode[6];
String sInte;
String alarms = "";
int prev_angle = 0;
int buzzDelay;
int buzzFreq{100};
int buzzMode;
int keepPlaying;
void setup() {
  Serial.begin(115200);
  setupWifi();
  setupMqtt();
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  servoMotor.attach(SERVO_PIN, 500, 2400);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  digitalWrite(LED_BUILTIN, LOW);

  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Medibox Online");
  LCD.setCursor(0, 1);
  LCD.print("Connecting to wifi");
  configTime(5 * 3600, 30 * 60, NTP_SERVER);
  delay(1000);
  LCD.clear();

}

void loop() {
  if(!mqttClient.connected())
  {
    connectToBroker();
  }
  mqttClient.loop();
  if(keepPlaying == 1)
  {
    startAlarm();
  }
  //tone(BUZZER_PIN, 100, 1000);
  printLocalTime();
  updateTempHumi();
  readIntensity();
  Serial.println(tempAr);
  mqttClient.publish("Medibox Temp", tempAr);
  mqttClient.publish("Medibox Humi", humiAr);
  mqttClient.publish("Medibox Inte", sInte.c_str());
  delay(1000);
}


void setupWifi(){
  WiFi.begin("Wokwi-GUEST", "");
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
}


void setupMqtt()
{
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
}

void connectToBroker()
{
  while(!mqttClient.connected())
  {
    Serial.print("Attemping MQTT connection...");
    if(mqttClient.connect("ESP32-3242425"))
    {
      Serial.println("connected");
      mqttClient.subscribe("Medibox light");
      mqttClient.subscribe("Medibox motor");
      mqttClient.subscribe("Medibox buzDelay");
      mqttClient.subscribe("Medibox buzFreq");
      mqttClient.subscribe("Medibox buzMode");
      mqttClient.subscribe("Medibox stopAlarm");
      mqttClient.subscribe("Medibox startAlarm");
      mqttClient.subscribe("Medibox active alarms");

    }
    else
    {
      Serial.print("failed ");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }
}

void updateTempHumi()
{
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(tempAr, 6);
  String(data.humidity, 2).toCharArray(humiAr, 6);
}

void readIntensity()
{
  int LDR_reading = analogRead(LDR_PIN);
  float intensity = 1 - LDR_reading/4095.0;
  Serial.print("intensity: ");
  Serial.println(intensity);
  sInte = String(intensity);
}
void startAlarm()
{
  if(buzzMode == 0)
  {
    for(int i = 0; i < 10; i++)
    {
      tone(BUZZER_PIN, buzzFreq+10*i, 300);
      delay(buzzDelay*1000);
    }
  }
  else{
    tone(BUZZER_PIN, buzzFreq, 1000);

  }
}

void printLocalTime() 
{
  setenv("TZ", "Asia/Colombo", 1);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    LCD.setCursor(0, 1);
    LCD.println("Connection Err");
    return;
  }
  LCD.setCursor(0, 0);
  LCD.println(&timeinfo, "%H:%M:%S");

  LCD.setCursor(0, 1);
  LCD.println(&timeinfo, "%d/%m/%Y   %Z");

  LCD.setCursor(0, 2);
  LCD.println("Active alarms");
  
  LCD.setCursor(0, 3);
  LCD.println("");
  LCD.println(alarms);

}

void receiveCallback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Messeage arrive [");
  Serial.print(topic);
  Serial.print("]\n");

  char payloadCharAr[length];
  for(int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }
  if(strcmp(topic, "Medibox light")==0)
  {
    if(payloadCharAr[0] == '1')
    {
        digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
        digitalWrite(LED_BUILTIN, LOW);
    }
  }
  else if(strcmp(topic, "Medibox motor")==0)
  {
    int motor_angle = atoi(payloadCharAr);
    int angle_difference = motor_angle-prev_angle;
    servoMotor.write(motor_angle);
    delay(100);
    Serial.print("motor angle: ");
    Serial.println(motor_angle);
  }
  else if(strcmp(topic, "Medibox buzDelay")==0)
  {
    buzzDelay = (int) atof(payloadCharAr);
  }
  else if(strcmp(topic, "Medibox buzFreq")==0)
  {
    buzzFreq = (int) atof(payloadCharAr);
  }
  else if(strcmp(topic, "Medibox startAlarm")==0)
  {
    keepPlaying  = 1;
  }
  else if(strcmp(topic, "Medibox stopAlarm")==0)
  {
    keepPlaying  = 0;
  }
  else if(strcmp(topic, "Medibox buzMode")==0)
  {
    buzzMode = (int) atof(payloadCharAr);
    tone(BUZZER_PIN, 2000, 100);
  }
  else if(strcmp(topic, "Medibox active alarms")==0)
  {
    alarms = "";
    for (int i = 0; i < length; i++) {
      alarms += (char)payloadCharAr[i];
    }
 
    delay(100);
    Serial.print("Active alarms ");
    Serial.println(alarms);
  }  
}
