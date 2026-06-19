#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"

// --- OLED Display Configuration ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Hardware Pin Configurations ---
#define PIN_HEART_RATE  34
#define PIN_SPO2        35
#define PIN_BODY_TEMP   25
#define PIN_DHT_ROOM    26
#define PIN_AQI         32
#define PIN_O2_LEVEL    33
#define PIN_BUZZER      27
#define PIN_ALERT_LED   4   // LED Connected to Digital GPIO 4

#define DHTTYPE DHT22
DHT dht(PIN_DHT_ROOM, DHTTYPE);

// --- Wokwi Virtual Wi-Fi Settings ---
const char* WLAN_SSID = "Wokwi-GUEST";
const char* WLAN_PASS = "";

// --- Adafruit IO Setup (Your Updated Credentials) ---
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "raj12"
#define AIO_KEY         "aio_aoWg38lvT63BnR7e1ahm5WBTH67U"

// Create WiFi and MQTT Client Instances
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// --- MQTT Feeds Setup ---
// Dashboard 1: Medical Staff Feeds (Role-Based Topic A)
Adafruit_MQTT_Publish feed_heart_rate = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/heart-rate");
Adafruit_MQTT_Publish feed_spo2       = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/spo2");
Adafruit_MQTT_Publish feed_body_temp  = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/body-temp");

// Dashboard 2: Facility Management Feeds (Role-Based Topic B)
Adafruit_MQTT_Publish feed_room_temp  = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/room-temp");
Adafruit_MQTT_Publish feed_aqi        = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/aqi");
Adafruit_MQTT_Publish feed_o2_level   = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/o2-level");

// --- Periodic Aggregation Timing Variables ---
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 5000; // Cloud push aggregate window (5 seconds)

void MQTT_connect();

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_ALERT_LED, OUTPUT); 
  
  dht.begin();

  // Initialize OLED Display via I2C Communication Bus (Address 0x3C)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("System Initializing...");
  display.display();

  // Connect to Wokwi Virtual Network Access Point
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected!");
}

void loop() {
  // Guard clause ensuring steady MQTT link state
  MQTT_connect();

  // --- 1. Read Edge Layer Sensors ---
  int raw_hr = analogRead(PIN_HEART_RATE);
  int heartRate = map(raw_hr, 0, 4095, 60, 150); // Maps analog volts to real human pulse range

  int raw_spo2 = analogRead(PIN_SPO2);
  int spo2 = map(raw_spo2, 0, 4095, 85, 100);    // Maps to blood oxygen saturation percentage

  int raw_body = analogRead(PIN_BODY_TEMP);
  float bodyTemp = 36.5 + (raw_body * 0.002);    // Calibrates analog signal to body temperature scale

  float roomTemp = dht.readTemperature();        // Pulls environmental temperature from DHT22 module
  
  int raw_aqi = analogRead(PIN_AQI);
  int aqi = map(raw_aqi, 0, 4095, 0, 500);       // Maps to universal Air Quality Index scale

  int raw_o2 = analogRead(PIN_O2_LEVEL);
  int o2Level = map(raw_o2, 0, 4095, 15, 25);    // Maps to room ambient atmospheric O2 levels

  // --- 2. Local Safety Critical Execution & Alarms ---
  if (spo2 < 90 || heartRate > 130 || aqi > 300 || bodyTemp > 38.0) {
    digitalWrite(PIN_BUZZER, HIGH);    // Trigger physical audio alert
    digitalWrite(PIN_ALERT_LED, HIGH); // Fire bright indicator warning LED
  } else {
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_ALERT_LED, LOW);
  }

  // --- 3. Render Local Screen Matrix Layout ---
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("--- PATIENT ROOM 01 ---"); // Device Location Identity Header
  
  display.setCursor(0, 18);
  display.print("Pulse Rate: "); display.print(heartRate); display.println(" BPM");
  
  display.setCursor(0, 32);
  display.print("SpO2 Level: "); display.print(spo2); display.println(" %");
  
  display.setCursor(0, 46);
  display.print("Body Temp : "); display.print(bodyTemp, 1); display.println(" C");
  
  // Footer Warning Flag Logic
  if (spo2 < 90 || heartRate > 130) {
    display.setCursor(0, 56);
    display.println("STATUS: !!CRITICAL!!");
  } else {
    display.setCursor(0, 56);
    display.println("STATUS: Normal");
  }
  display.display();

  // --- 4. Enterprise Data Pipeline Cloud Publishing Routing ---
  if (millis() - lastPublishTime >= publishInterval) {
    lastPublishTime = millis();

    Serial.println("\n--- Aggregating & Publishing Telemetry ---");

    // Route Packets to Dashboard A: Medical Staff Feeds (Casted via int32_t)
    if (feed_heart_rate.publish((int32_t)heartRate)) { Serial.print("Published HR: "); Serial.println(heartRate); }
    if (feed_spo2.publish((int32_t)spo2))             { Serial.print("Published SpO2: "); Serial.println(spo2); }
    if (feed_body_temp.publish(bodyTemp))             { Serial.print("Published Body Temp: "); Serial.println(bodyTemp); }

    // Route Packets to Dashboard B: Facility Management Feeds
    if (!isnan(roomTemp)) {
      if (feed_room_temp.publish(roomTemp))           { Serial.print("Published Room Temp: "); Serial.println(roomTemp); }
    }
    if (feed_aqi.publish((int32_t)aqi))               { Serial.print("Published AQI: "); Serial.println(aqi); }
    if (feed_o2_level.publish((int32_t)o2Level))     { Serial.print("Published O2 Level: "); Serial.println(o2Level); }
  }
}

// Resilient MQTT Broker Handshake Core Function
void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) return;

  Serial.print("Connecting to Adafruit IO MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { 
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);
       retries--;
       if (retries == 0) {
         while (1); // Safe trap state for automated edge resets
       }
  }
  Serial.println("Adafruit IO Connected!");
}
