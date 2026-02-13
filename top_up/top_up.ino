#include <ESP8266WiFi.h>
#include <PubSubClient.h> 
#include <SPI.h>
#include <MFRC522.h>     
#include <ArduinoJson.h> 
#include <map>

// --- Configuration ---
const char* ssid = "EdNet";
const char* password = "Huawei@123";
const char* mqtt_server = "157.173.101.159";
const char* team_id = "poiuy";

String topic_status  = "rfid/poiuy/card/status";
String topic_topup   = "rfid/poiuy/card/topup";
String topic_balance = "rfid/poiuy/card/balance";

// --- Global State ---
std::map<String, int> card_ledger; 

#define SS_PIN 5   // D1
#define RST_PIN 4  // D2
MFRC522 mfrc522(SS_PIN, RST_PIN); 

WiFiClient espClient;
PubSubClient client(espClient);

// ------------------- MQTT Callback -------------------
void callback(char* topic, byte* payload, unsigned int length) {

  Serial.println("\n[MQTT] Message received!");
  Serial.print("[MQTT] Topic: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("[ERROR] JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  String uid = doc["uid"].as<String>();
  int topup_amount = doc["amount"];

  Serial.print("[TOPUP] UID: ");
  Serial.print(uid);
  Serial.print(" | Amount: ");
  Serial.println(topup_amount);

  // Update balance for this UID
  card_ledger[uid] += topup_amount;

  Serial.printf("[DEBUG] New balance for %s = %d\n", uid.c_str(), card_ledger[uid]);

  // Notify backend/dashboard
  StaticJsonDocument<200> response;
  response["uid"] = uid;
  response["new_balance"] = card_ledger[uid];

  char buffer[256];
  serializeJson(response, buffer);

  bool ok = client.publish(topic_balance.c_str(), buffer);

  if (ok) {
    Serial.println("[MQTT] Balance published successfully!");
  } else {
    Serial.println("[ERROR] Failed to publish balance!");
  }
}

// ------------------- WiFi Connection -------------------
void connectWiFi() {
  Serial.println("\n[WiFi] Connecting to WiFi...");
  Serial.print("[WiFi] SSID: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[WiFi] Connected successfully!");
  Serial.print("[WiFi] IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.print("[WiFi] Signal Strength (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

// ------------------- MQTT Connection -------------------
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("\n[MQTT] Connecting to broker...");
    Serial.print("[MQTT] Server: ");
    Serial.println(mqtt_server);

    if (client.connect(team_id)) {
      Serial.println("[MQTT] Connected successfully!");

      Serial.print("[MQTT] Subscribing to: ");
      Serial.println(topic_topup);

      client.subscribe(topic_topup.c_str());

      Serial.println("[MQTT] Subscription done!");
    } else {
      Serial.print("[ERROR] MQTT connection failed. rc=");
      Serial.println(client.state());
      Serial.println("[MQTT] Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// ------------------- Setup -------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========== RFID TOP-UP SYSTEM STARTED ==========");

  // WiFi
  connectWiFi();

  // SPI + RFID
  Serial.println("[RFID] Initializing RFID reader...");
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("[RFID] RFID reader initialized!");

  // MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// ------------------- Loop -------------------
void loop() {

  // Ensure WiFi stays connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Disconnected! Reconnecting...");
    connectWiFi();
  }

  // Ensure MQTT stays connected
  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  // RFID card scanning
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {

    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    Serial.println("\n[RFID] Card detected!");
    Serial.print("[RFID] UID: ");
    Serial.println(uid);

    int my_balance = card_ledger[uid];

    Serial.print("[RFID] Current balance: ");
    Serial.println(my_balance);

    StaticJsonDocument<200> doc;
    doc["uid"] = uid;
    doc["balance"] = my_balance;

    char buffer[256];
    serializeJson(doc, buffer);

    bool ok = client.publish(topic_status.c_str(), buffer);

    if (ok) {
      Serial.println("[MQTT] Card status published successfully!");
    } else {
      Serial.println("[ERROR] Failed to publish card status!");
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}
