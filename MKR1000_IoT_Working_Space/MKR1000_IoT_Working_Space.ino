#define BLYNK_TEMPLATE_ID "TMPLVzbHHHYj"
#define BLYNK_DEVICE_NAME "IoT Working Space"
#define BLYNK_AUTH_TOKEN "zmFpMhwKFk1N0gjtJOte95wKcwC9B6WG"
#define BLYNK_PRINT Serial
#define DATABASE_URL "test-5a42b-default-rtdb.asia-southeast1.firebasedatabase.app"
#define DATABASE_SECRET "G8TMr0HZlm5buQZ8gJpdpQXr3d6lqmKH4KpPRLn6"
#define APP_KEY           "47f05c0b-8f2c-4b74-a5d2-c00aec445c57" 
#define APP_SECRET        "4b9fa934-ee9b-4fd0-8a7c-c4a3b9807804-e696b366-91f3-47a6-b2f9-c8f3489cf562"
#define SWITCH_ID         "638420f9b8a7fefbd64c1f37"

#ifdef ENABLE_DEBUG
       #define DEBUG_ESP_PORT Serial
       #define NODEBUG_WEBSOCKETS
       #define NDEBUG
#endif 

#include <SPI.h>
#include <WiFi101.h>
#include <WiFiUdp.h>
#include <RTCZero.h>
#include "SinricPro_Generic.h"
#include "SinricProSwitch.h"
#include <BlynkSimpleMKR1000.h>
#include <Firebase_Arduino_WiFi101.h>
#include <time.h>
#include <Servo.h>
#include <IRremote.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

/*****************************************************************/
//Firebase
FirebaseData firebaseData;
/*****************************************************************/

/*****************************************************************/
// Object for Real Time Clock
RTCZero rtc;
// Time zone constant - change as required for your location
const int GMT = +8; 

int hours, mins, secs;
int Day, month;
String Month, Year;
String lcdHours, lcdMins, lcdSeconds;
int check_out_hours;
int check_out_mins;
unsigned long Started = 0;
const long interval = 1000;
/*****************************************************************/

/*****************************************************************/
//Virtual Pin
int led1;
int led2;
int led3;
int on_ir;
/*****************************************************************/

/*****************************************************************/
// LCD
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  
/*****************************************************************/

/*****************************************************************/
// Oled
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
/*****************************************************************/

/*****************************************************************/
//DHT
#define DHTPIN 5 
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);
/*****************************************************************/

/*****************************************************************/
//RFID
#define SS_PIN 6
#define RST_PIN 7
MFRC522 mfrc522(SS_PIN, RST_PIN);
/*****************************************************************/

/*****************************************************************/
//MISC
#define buzzer A2
#define led_red A1
#define servoPin 3
#define PIN_SEND 4
#define led_1 18 
#define led_2 19   
#define led_3 20
Servo myservo;
/*****************************************************************/

/*****************************************************************/
char authen[] = BLYNK_AUTH_TOKEN;
char ssid[] = "semanz";
char pass[] = "33632407";
/*****************************************************************/

/*****************************************************************/
//Blynk Virtual Pins
BLYNK_WRITE(V2) {
  led1 = param.asInt(); // assigning incoming value from pin V2 to a variable
}

BLYNK_WRITE(V3) {
  led2 = param.asInt(); 
}

BLYNK_WRITE(V4) {
  led3 = param.asInt(); 
}

BLYNK_WRITE(V5) {
  on_ir = param.asInt();
  if (on_ir == 1){
    myservo.detach();
    IrSender.sendNEC(0xFFE01F, 32);
    delay(100);
    IrSender.sendNEC(0xFFE01F, 32);
    delay(100);
    IrSender.sendNEC(0xFFE01F, 32);
    Serial.println("ON");    
  }
  else if (on_ir == 0){
    myservo.detach();
    IrSender.sendNEC(0xFF609F, 32);
    delay(100);
    IrSender.sendNEC(0xFF609F, 32);
    delay(100);
    IrSender.sendNEC(0xFF609F, 32);
    Serial.println("OFF");    
  } 
}
/*****************************************************************/

void setupSinricPro(){
  SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];   // create new switch device
  mySwitch.onPowerState(onPowerState);                // apply onPowerState callback
  SinricPro.begin(APP_KEY, APP_SECRET);               // start SinricPro
}

void setupWiFi(){
  /*****************************************************************/
  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  /*****************************************************************/
}

void setupLCD(){
  //LCD Setup
  lcd.begin();                                                            
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("IoT Working");
  lcd.setCursor(5,1);
  lcd.print("Space");
}

void setupOLED(){
  //OLED Setup
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
}

void setupMISC(){
  SPI.begin();          
  dht.begin();
  mfrc522.PCD_Init();        
  myservo.attach(servoPin);

  IrSender.begin(PIN_SEND);     
  Blynk.begin(authen, ssid, pass);
  Firebase.begin(DATABASE_URL, DATABASE_SECRET, ssid, pass);
  Firebase.reconnectWiFi(true);
}

void setupLED(){
  pinMode(led_1,OUTPUT); 
  pinMode(led_2,OUTPUT); 
  pinMode(led_3,OUTPUT); 
  pinMode(led_red,OUTPUT); 
}

void setupNTP(){
  // Start Real Time Clock
  rtc.begin();
  
  // Variable to represent epoch
  unsigned long epoch;
 
 // Variable for number of tries to NTP service
  int numberOfTries = 0, maxTries = 6;

 // Get epoch
  do {
    epoch = WiFi.getTime();
    numberOfTries++;
  }

  while ((epoch == 0) && (numberOfTries < maxTries));

    if (numberOfTries == maxTries) {
    Serial.print("NTP unreachable!!");
    while (1);
    }

    else {
    Serial.print("Epoch received: ");
    Serial.println(epoch);
    rtc.setEpoch(epoch);
    Serial.println();
    }
}

void setup() {
  Serial.begin(9600);
  setupWiFi();
  setupSinricPro();
  setupLCD();
  setupOLED();
  setupMISC();
  setupLED();
  setupNTP();
}

// Get Time from RTC
void printTime(){
  hours = rtc.getHours() + GMT;
  mins = rtc.getMinutes();
  secs = rtc.getSeconds();
  lcdHours = String(hours);
  lcdMins = String(mins);
  lcdSeconds = String(secs);
  lcd.clear();  
  lcd.setCursor(0,1);
  lcd.print(lcdHours + ":" + lcdMins + ":" + lcdSeconds);
}

// Get Date from RTC
void printDate(){ 
  month = rtc.getMonth();
  switch (month){
    case 1:
      Month = " January";
    case 2:
      Month = " February";
    case 3:
      Month = " March";
    case 4:
      Month = " April";
    case 5:
      Month = " May";
    case 6:
      Month = " June";
    case 7:
      Month = " July";
    case 8:
      Month = " August";
    case 9:
      Month = " September";
    case 10:
      Month = " October";
    case 11:
      Month = " November";
    case 12:
      Month = " December";
    default:
      break;
  }
  Day = rtc.getDay();

  Year = (" 20" + String(rtc.getYear()));
  lcd.setCursor(0,0);
  lcd.print(Day + Month + Year);
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}

// Set Time to Run Every 1 Sec
void timer() {
  unsigned long currentMillis = millis();
  if (currentMillis - Started >= interval) {
    Started = currentMillis;
    printTime();
    printDate();
  }
  else{}
}

// Get Check Out Time (Hours and Minutes) From Users - Firebase
void getTime(){
  if (Firebase.getInt(firebaseData, "/TIME/HOURS")){
    if (firebaseData.dataType() == "int"){
      check_out_hours = (firebaseData.intData());
    }
  }
  if (Firebase.getInt(firebaseData, "/TIME/MINUTES")){
    if (firebaseData.dataType() == "int"){
      check_out_mins = (firebaseData.intData());
    }
  }
}

void controlvLED1(){
  BLYNK_WRITE(led1);
  if (led1 == 1 && mins != check_out_mins){
    digitalWrite(led_1,HIGH);
    Firebase.setString(firebaseData, "/MKR1000_LAMP/LAMP_1", "ON");
  }
    else if (led1 == 0 && mins != check_out_mins){
      digitalWrite(led_1,LOW);
      Firebase.setString(firebaseData, "/MKR1000_LAMP/LAMP_1", "OFF");
    }
}

void controlvLED2(){
  BLYNK_WRITE(led2);
  if (led2 == 1 && mins != check_out_mins){
    digitalWrite(led_2,HIGH);
    Firebase.setString(firebaseData, "/MKR1000_LAMP/LAMP_2", "ON");
  }
    else if (led2 == 0 && mins != check_out_mins){
      digitalWrite(led_2,LOW);
      Firebase.setString(firebaseData, "/MKR1000_LAMP/LAMP_2", "OFF");
    }
}

void controlvLED3(){
  BLYNK_WRITE(led3);
  if (led3 == 1 && mins != check_out_mins){
    digitalWrite(led_3,HIGH);
    Firebase.setString(firebaseData, "/MKR1000_LAMP/LAMP_3", "ON");
  }
    else if (led3 == 0 && mins != check_out_mins){
      digitalWrite(led_3,LOW);
      Firebase.setString(firebaseData, "/MKR1000_LAMP/LAMP_3", "OFF");
    }
}

// Control LEDs From V-Pin
void control(){
  controlvLED1(); 
  controlvLED2();
  controlvLED3();
}

//Control IR Lamp using Google Assistant
bool onPowerState(const String &deviceId, bool &state) {
  digitalWrite(PIN_SEND, state); 
  if(state){
    IrSender.sendNEC(0xFFE01F, 32);
    Firebase.setString(firebaseData, "/MKR1000_LAMP/ROOM_LIGHT", "ON"); 
    IrSender.sendNEC(0xFFE01F, 32);
    IrSender.sendNEC(0xFFE01F, 32);   
  }    
  else if (!state){
    IrSender.sendNEC(0xFF609F, 32);
    Firebase.setString(firebaseData, "/MKR1000_LAMP/ROOM_LIGHT", "OFF");
    IrSender.sendNEC(0xFF609F, 32);
    IrSender.sendNEC(0xFF609F, 32);
  }
  return true;
}

// Autherized Access - RFID
void auth_acc(){
    myservo.attach(servoPin);
    Serial.println("Authorized access");
    Serial.println();
    Firebase.setString(firebaseData, "/ESP32_APP/RFID", "Permitted");

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Welcome Home");
    myservo.write(180);
    digitalWrite(led_red, HIGH);
    tone(buzzer , 5000) ;
    delay(300) ;
  	noTone(buzzer) ; 
    delay(2000);
    digitalWrite(led_red, LOW);
    myservo.write(0);
}

// Unautherized Access - RFID
void unauth_acc(){
    Serial.println("Access Denied");
    Serial.println();
    Firebase.setString(firebaseData, "/ESP32_APP/RFID", "Denied");

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Sorry Wrong Key");
    digitalWrite(led_red, HIGH);
    tone(buzzer,5000);
    delay(300);
    digitalWrite(led_red, LOW);
  	noTone(buzzer);
    delay(300);
    digitalWrite(led_red, HIGH);
    tone(buzzer,5000);
    delay(300);
    digitalWrite(led_red, LOW);
    noTone(buzzer);
}

// RFID
void rfid_door() {
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  content.toUpperCase();
  if (content.substring(1) == "D2 57 D5 1A") //change here the UID of the card/cards that you want to give access
  {
    auth_acc();  
  }
 
  else   
  {
    unauth_acc();
  }
}

// Receive and Send Data from DHT 11 to Firebase + Display on Oled
void dht_oled() {
  //read temperature and humidity
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
  }

  // clear display
  display.clearDisplay();
  
  Blynk.virtualWrite(V1, t);
  Blynk.virtualWrite(V0, h);

  /*****************************************************************/ //Sambung sini
  //Firebase
  Firebase.setFloat(firebaseData, "/MKR1000_DHT11/TEMPERATURE", t);
  Firebase.setFloat(firebaseData, "/MKR1000_DHT11/HUMIDITY", h);
  /*****************************************************************/

  // display temperature
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Temperature: ");
  display.setTextSize(2);
  display.setCursor(0,10);
  display.print(t);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");
  
  // display humidity
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("Humidity: ");
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print(h);
  display.print(" %"); 
  
  display.display();
}

// All LEDs ON
void led_on(){
    digitalWrite(led_1,HIGH);
    digitalWrite(led_2,HIGH);
    digitalWrite(led_3,HIGH);
}

// All LEDs OFF
void led_off(){
    digitalWrite(led_1,LOW);
    digitalWrite(led_2,LOW);
    digitalWrite(led_3,LOW);
}

// Run Voice Control During Active Hours Only
void checkoutControl(){
  if (mins != check_out_mins){
    SinricPro.handle();
  }
  else {
    Serial.println("OFF ALL");
    led_off();
  }
}

void loop() {
  timer();
  getTime();
  control();
  Blynk.run();
  checkoutControl();
  dht_oled();
  rfid_door();
}

