#include <WiFi.h>
#include <FirebaseESP32.h>
#include <WiFiManager.h>

// ===== HC-SR04 пинове =====
#define TRIG_PIN_1 5
#define ECHO_PIN_1 18   // ⚠️ Разкачи при upload, ако се получават Write timeout

#define TRIG_PIN_2 21
#define ECHO_PIN_2 19

#define TRIG_PIN_3 23
#define ECHO_PIN_3 22

#define OCCUPIED_DISTANCE 20   // под 20 см = ЗАЕТО

// ===== Firebase =====
#define FIREBASE_HOST "esp32-5d620-default-rtdb.firebaseio.com"
#define DATABASE_SECRET "3QUGHinqLFgAMrpHrP7xq7nrKOQiywCt80fGvLFq"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===== Таймер =====
unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 500; // ms

// ===== Функция за измерване =====
long measureDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 25000);
  if (duration == 0) return -1;

  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== СТАРТ НА СИСТЕМАТА ===");

  // ===== Настройка на пиновете =====
  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);

  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);

  pinMode(TRIG_PIN_3, OUTPUT);
  pinMode(ECHO_PIN_3, INPUT);

  // ===== WiFiManager =====
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);

  if (!wifiManager.autoConnect("ESP32_Setup", "12345678")) {
    Serial.println("WiFi грешка. Рестарт...");
    delay(2000);
    ESP.restart();
  }

  Serial.println("WiFi свързан");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // ===== Firebase =====
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = DATABASE_SECRET;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase готов");
}

void loop() {

  // ===== Измервания (малко закъснение за да няма смущения) =====
  long distance1 = measureDistanceCM(TRIG_PIN_1, ECHO_PIN_1);
  delay(50);

  long distance2 = measureDistanceCM(TRIG_PIN_2, ECHO_PIN_2);
  delay(50);

  long distance3 = measureDistanceCM(TRIG_PIN_3, ECHO_PIN_3);
  delay(50);

  String status1 = (distance1 > 0 && distance1 <= OCCUPIED_DISTANCE) ? "ЗАЕТО" : "ПРАЗНО";
  String status2 = (distance2 > 0 && distance2 <= OCCUPIED_DISTANCE) ? "ЗАЕТО" : "ПРАЗНО";
  String status3 = (distance3 > 0 && distance3 <= OCCUPIED_DISTANCE) ? "ЗАЕТО" : "ПРАЗНО";

  // ===== Serial Monitor =====
  Serial.println("\n=== ПАРКИНГ ===");
  Serial.printf("Място 1: %ld см → %s\n", distance1, status1.c_str());
  Serial.printf("Място 2: %ld см → %s\n", distance2, status2.c_str());
  Serial.printf("Място 3: %ld см → %s\n", distance3, status3.c_str());

  // ===== Firebase (една заявка с JSON) =====
  if (millis() - lastSend >= SEND_INTERVAL &&
      WiFi.status() == WL_CONNECTED &&
      Firebase.ready()) {

    lastSend = millis();

    FirebaseJson json;
    json.set("spot1/distance", distance1);
    json.set("spot1/status", status1);

    json.set("spot2/distance", distance2);
    json.set("spot2/status", status2);

    json.set("spot3/distance", distance3);
    json.set("spot3/status", status3);

    if (!Firebase.setJSON(fbdo, "/parking", json)) {
      Serial.println("Firebase грешка: " + fbdo.errorReason());
    }
  }

  // ===== Серийни команди =====
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "resetwifi") {
      Serial.println("Reset WiFi...");
      WiFiManager wifiManager;
      wifiManager.resetSettings();
      delay(2000);
      ESP.restart();
    }
  }

  delay(50);
}