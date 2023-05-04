#include <ESP32Servo.h>
#include <LiquidCrystal.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  21  // ESP32 pin GPIO21
#define RST_PIN 22 // ESP32 pin GPIO22 

MFRC522 mfrc522(SS_PIN, RST_PIN);

// network credentials
const char* ssid = "Narzo";
const char* password = "narcotics";

// Initializing Telegram BOT
#define BOTtoken "5995373119:AAH_Z750KAxMXxUifsoeOrAGX5E1Ti5jtX4"  // Bot Token (Recieved from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
#define CHAT_ID "1345756349"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

LiquidCrystal lcd(4, 13, 5, 17, 16, 15);

#define MOTION_SENSOR_PIN  14 // ESP32 pin GIOP14 connected to motion sensor's pin
#define SERVO_PIN          26 // ESP32 pin GIOP26 connected to servo motor's pin

Servo servo; // creating servo object to control a servo

// variables will change:
int angle = 120;          // the current angle of servo motor
int currentMotionState; // the current state of motion sensor

int Ledyellow = 25;
int LDR = 34;
int nilaiLDR = 0;

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your cabinet.\n\n";
      welcome += "/state to request current Gate state \n";
      bot.sendMessage(chat_id, welcome, "");
    }
    
    if (text == "/state") {
      if (digitalRead(25)){
        bot.sendMessage(chat_id, "Gate is open", "");
      }
      else{
        bot.sendMessage(chat_id, "Gate is closed", "");
      }
    }
  }
}
//Function when authorized person access the door
void access() {
  Serial.print("Accessed message sending....");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  String user = content.substring(1) + " Accessed the Cabinet." + ".\n";
  bot.sendMessage(CHAT_ID, user, "");
  Serial.print("Accessed message sent!");
  Serial.println("");
}

//Function when unauthorized person tries to open the cabinet
void denied() {
  Serial.print("Denial message sending....");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  String user = content.substring(1) + " Tried to access Cabinet." + ".\n";
  bot.sendMessage(CHAT_ID, user, "");
  Serial.print("Denial message sent");
  Serial.println("");
}


void setup() {
  Serial.begin(9600);                // initialize serial

  SPI.begin(); // init SPI bus
  mfrc522.PCD_Init(); // init MFRC522

  pinMode(MOTION_SENSOR_PIN, INPUT); // set ESP32 pin to input mode
  servo.attach(SERVO_PIN);           // attaches the servo on pin 9 to the servo object

  servo.write(angle);
  currentMotionState = digitalRead(MOTION_SENSOR_PIN);

  pinMode(Ledyellow, OUTPUT);  

  lcd.begin(16, 2);

  //connect to wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
}

void loop() {

  lcd.clear();
  //Telegram new message handler
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  // Motion detection
  currentMotionState = digitalRead(MOTION_SENSOR_PIN); // read new state

  if (currentMotionState == HIGH) { // pin state change: LOW -> HIGH
    Serial.println("Motion detected!°");
    lcd.setCursor(0, 0);
    lcd.print("Hello (^_^)");
    lcd.setCursor(0, 1);
    lcd.print("Punch your RFID!");
    delay(3000);
    // Looking for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
      return;
    }
    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      return;
    }
    //Show UID on serial monitor
    Serial.print("UID tag :");
    String content= "";
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++) 
    {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.println();
    Serial.print("Message : ");
    content.toUpperCase();

  /*--------------------------------------------------------- Authorized ---------------------------------------------------------*/
  
    if (content.substring(1) == "4A B4 2D 48" || content.substring(1) == "E9 7E 8F 36") //change here the UID of the card/cards that you want to give access
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Access Granted ►");
      Serial.println("Authorized access");
      Serial.println();
      for(angle = 120; angle >= 0; angle-=1){
        servo.write(angle);
        delay(30);
      }
      /*-------------------------------------------LDR + LED --------------------------------*/
      nilaiLDR=analogRead(LDR);
      Serial.print("Nilai LDR =");
      Serial.println(nilaiLDR);

      if (nilaiLDR < 500){
        digitalWrite(Ledyellow,HIGH);
      }
      else if (nilaiLDR > 500 ){ 
        digitalWrite(Ledyellow,LOW);
      }
      access();

      delay(10000);
      for(angle = 0; angle <= 120; angle+=1){
        servo.write(angle);
        delay(30);
      }
      digitalWrite(Ledyellow, LOW);
    }
      /*------------------------ Denied ----------------------*/
    else   {
      Serial.println(" Access denied ❌");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Access denied");
      denied();
      Serial.print("");
    }
    delay(2000);
  }
}
