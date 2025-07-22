// Firebase + MQ135 + DHT22 + BMP180 + OLED (ESP32)
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP085.h>
#include <time.h>

#define WIFI_SSID "Shakthi"
#define WIFI_PASSWORD "15120909"

#define MQ135_PIN 4
#define DHT_PIN 15
#define DHT_TYPE DHT22

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define API_KEY "AIzaSyCWAZuBvel0nkdu3jLvSOPiBMijkqgtW2I"
#define DATABASE_URL "https://tempdatalog-a06cb-default-rtdb.firebaseio.com"
#define USER_EMAIL "poggyztsi0101@gmail.com"
#define USER_PASSWORD "Sree@0101"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
const unsigned long firebaseInterval = 60000;

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_BMP085 bmp;
bool hasBMP = false;

int safeAnalogRead(uint8_t pin) {
  WiFi.mode(WIFI_OFF);
  delay(50);
  int val = analogRead(pin);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(100);
  return val;
}

int readAirQuality() {
  return safeAnalogRead(MQ135_PIN);
}

String getFormattedTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "Unknown";
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);
}

String getFirebaseSafeTimestamp() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "unknown_time";
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &timeinfo);
  return String(buf);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // IST timezone

  pinMode(MQ135_PIN, INPUT);
  dht.begin();

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED not detected. Continuing without display."));
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Display initialized");
    display.display();
  }

  if (bmp.begin()) {
    Serial.println("BMP180 detected.");
    hasBMP = true;
  } else {
    Serial.println("BMP180 not detected.");
  }
}

void loop() {
  int airQuality = readAirQuality();
  Serial.print("Air Quality: ");
  Serial.println(airQuality);

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float pressure = NAN;
  float altitude = NAN;

  if (!isnan(temperature) && !isnan(humidity)) {
    Serial.print("Temp: "); Serial.println(temperature);
    Serial.print("Humidity: "); Serial.println(humidity);
  }

  if (hasBMP) {
    long pressureRaw = bmp.readPressure();
    if (pressureRaw > 0) {
      pressure = pressureRaw / 100.0F;  // Convert Pa to hPa
      altitude = bmp.readAltitude(1013.25);  // Sea level pressure (adjust as needed)
      Serial.print("Pressure: "); Serial.println(pressure);
      Serial.print("Altitude: "); Serial.println(altitude);
    } else {
      Serial.println("Failed to read pressure from BMP180");
    }
  }

  if (WiFi.status() == WL_CONNECTED && Firebase.ready() && (millis() - sendDataPrevMillis >= firebaseInterval)) {
    sendDataPrevMillis = millis();

    FirebaseJson json;
    json.set("air_quality", airQuality);
    json.set("timestamp", getFormattedTime());

    if (!isnan(temperature)) json.set("iiit_temperature", temperature);
    if (!isnan(humidity)) json.set("iiit_humidity", humidity);
    if (!isnan(pressure)) json.set("iiit_pressure", pressure);
    if (!isnan(altitude)) json.set("iiit_altitude", altitude);

    String timestampPath = getFirebaseSafeTimestamp();
    String fullPath = "/test/readings/" + timestampPath;

    Serial.println("Uploading to Firebase...");
    if (Firebase.RTDB.setJSON(&fbdo, fullPath.c_str(), &json)) {
      Serial.println("Data sent successfully to " + fullPath);
    } else {
      Serial.print("Firebase error: ");
      Serial.println(fbdo.errorReason());
    }
  }

  // OLED Display update
  if (!isnan(temperature) && !isnan(humidity)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("Temp: "); display.println(temperature);
    display.print("Hum: "); display.println(humidity);
    display.print("Air: "); display.println(airQuality);
    if (!isnan(pressure)) {
      display.print("Pres: "); display.print(pressure); display.println("hPa");
    }
    if (!isnan(altitude) && altitude > -1000 && altitude < 10000) {
      display.print("Alt: "); display.print(altitude); display.println("m");
    }
    display.display();
  }
}
