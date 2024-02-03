#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   
#include <ArduinoJson.h>
#include <MFRC522.h>
const char* ssid = "";
const char* password = "";
#define BOTtoken ".."
#define CHAT_ID ".."
#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif
#define SS_PIN D8
#define RST_PIN D0
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int bot_delay = 100;
unsigned long lastTimeBotRan;
const int ledPin = 2;
bool ledState = LOW;

MFRC522 rfid(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key key;


void handleNewMessages(int numNewMessages) {
  Serial.println("Handling New Message");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    String user_text = bot.messages[i].text;
    Serial.println(user_text);
    String your_name = bot.messages[i].from_name;

    if (user_text == "Hej") {
      String welcome = "Cześć, " + your_name + ".\n";
      welcome += "Napisz /stan żeby sprawdzić stan urzadzeń\n";
      welcome += "Napisz /on żeby załączyć diode\n";
      welcome += "Send /off żeby wyłączyć diode \n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (user_text == "/on") {
      bot.sendMessage(chat_id, "Włączam", "");
      ledState = LOW;
      digitalWrite(BUILTIN_LED, ledState);
    }
    
    if (user_text == "/off") {
      bot.sendMessage(chat_id, "Wyłączam", "");
      ledState = HIGH;
      digitalWrite(BUILTIN_LED, ledState);
    }
    
    if (user_text == "/stan") {
      if (digitalRead(BUILTIN_LED)){
        bot.sendMessage(chat_id, "Dioda wyłączona", "");
      }
      else{
        bot.sendMessage(chat_id, "Dioda załączona", "");
      }
    } 
  }
}

void setup() {
  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();
  
  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
    client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  #endif

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
}

void CZYTNIKLOOP()
{
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return;
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  String strID = "";
  for (byte i = 0; i < 4; i++) {
    strID +=
      (rfid.uid.uidByte[i] < 0x10 ? "0" : "") +
      String(rfid.uid.uidByte[i], HEX) +
      (i != 3 ? ":" : "");
  }

  strID.toUpperCase();
  Serial.print("Tap card key: ");
  Serial.println(strID);
  bot.sendMessage(CHAT_ID,"Zbliżono karte: " + strID, "");
  delay(bot_delay);
}

void loop() {
  
  if (millis() > lastTimeBotRan + bot_delay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("Got Response!");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  CZYTNIKLOOP();
}