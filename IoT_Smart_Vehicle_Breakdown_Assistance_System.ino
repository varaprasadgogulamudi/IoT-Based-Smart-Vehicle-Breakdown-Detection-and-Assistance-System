#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>

// =====================================================
// WIFI
// =====================================================

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// =====================================================
// THINGSPEAK
// =====================================================

const unsigned long CHANNEL_ID = 3492180;

// IMPORTANT:
// Put your NEW ThingSpeak WRITE API key here.
// Do not share the key with anyone.
const char* THINGSPEAK_API_KEY = "YOUR_THINGSPEAK_WRITE_API_KEY";

const char* THINGSPEAK_URL =
  "http://api.thingspeak.com/update";

// =====================================================
// LCD
// =====================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================================================
// GPS
// =====================================================

TinyGPSPlus gps;
HardwareSerial GPSserial(2);

#define GPS_RX 16
#define GPS_TX 17

// GPS TX -> GPIO16
// GPS RX -> GPIO17

// =====================================================
// L298N MOTOR DRIVER
// =====================================================

#define IN1 32
#define IN2 33
#define IN3 18
#define IN4 19

// =====================================================
// BUZZER
// =====================================================

#define BUZZER_PIN 25

// =====================================================
// OBSTACLE SENSOR
// =====================================================

#define OBSTACLE_PIN 26

// LOW  = OBSTACLE DETECTED
// HIGH = NO OBSTACLE

// =====================================================
// FUEL SWITCH
// =====================================================

#define FUEL_PIN 27

// LOW  = FUEL LOW
// HIGH = NORMAL

// =====================================================
// MPU6050
// =====================================================

#define MPU_ADDR 0x68

// =====================================================
// MOVEMENT / BREAKDOWN DETECTION
// =====================================================

const int MOVEMENT_THRESHOLD = 12000;

const int REQUIRED_ABNORMAL_READINGS = 5;

int abnormalCount = 0;
bool abnormalMovement = false;

unsigned long normalSince = 0;
const unsigned long AUTO_CLEAR_TIME = 10000;

int startupReadings = 0;

const int STARTUP_IGNORE_READINGS = 30;

// Previous MPU readings
int16_t previousAX = 0;
int16_t previousAY = 0;
int16_t previousAZ = 0;

// =====================================================
// MPU VALUES
// =====================================================

int16_t ax = 0;
int16_t ay = 0;
int16_t az = 0;

float totalAcceleration = 0;

long deltaAcceleration = 0;

// =====================================================
// SAFETY VALUES
// =====================================================

bool obstacleDetected = false;
bool fuelLow = false;

// =====================================================
// THINGSPEAK TIMER
// =====================================================

unsigned long lastThingSpeakUpload = 0;

const unsigned long THINGSPEAK_INTERVAL = 15000;

// =====================================================
// READ GPS
// =====================================================

void readGPS() {

  while (GPSserial.available() > 0) {

    char c = GPSserial.read();

    gps.encode(c);
  }
}

// =====================================================
// WRITE MPU REGISTER
// =====================================================

void writeMPU(uint8_t reg, uint8_t value) {

  Wire.beginTransmission(MPU_ADDR);

  Wire.write(reg);
  Wire.write(value);

  Wire.endTransmission();
}

// =====================================================
// READ MPU6050
// =====================================================

void readMPU() {

  Wire.beginTransmission(MPU_ADDR);

  Wire.write(0x3B);

  Wire.endTransmission(false);

  Wire.requestFrom(MPU_ADDR, 6);

  if (Wire.available() >= 6) {

    ax = Wire.read() << 8 | Wire.read();

    ay = Wire.read() << 8 | Wire.read();

    az = Wire.read() << 8 | Wire.read();
  }

  // ---------------------------------------------------
  // Calculate total acceleration
  // ---------------------------------------------------

  totalAcceleration = sqrt(
    (float)ax * ax +
    (float)ay * ay +
    (float)az * az
  );

  // ---------------------------------------------------
  // Calculate change from previous reading
  // ---------------------------------------------------

  deltaAcceleration =
    abs((long)ax - previousAX) +
    abs((long)ay - previousAY) +
    abs((long)az - previousAZ);

  // ---------------------------------------------------
  // Save current readings
  // ---------------------------------------------------

  previousAX = ax;
  previousAY = ay;
  previousAZ = az;

  // ---------------------------------------------------
  // Ignore first 30 readings
  // ---------------------------------------------------

  if (startupReadings < STARTUP_IGNORE_READINGS) {

    startupReadings++;

    abnormalCount = 0;

    return;
  }

  // ---------------------------------------------------
  // Movement / breakdown detection
  // ---------------------------------------------------

  if (!abnormalMovement) {

  if (deltaAcceleration > MOVEMENT_THRESHOLD) {

    abnormalCount++;
    normalSince = 0;

    Serial.print("HIGH MOVEMENT: ");
    Serial.print(abnormalCount);
    Serial.print("/");
    Serial.println(REQUIRED_ABNORMAL_READINGS);

    if (abnormalCount >= REQUIRED_ABNORMAL_READINGS) {

      abnormalMovement = true;
      normalSince = 0;

      Serial.println();
      Serial.println("!!! VEHICLE BREAKDOWN ALERT !!!");
      Serial.println();
    }

  } else {

    abnormalCount = 0;
  }

} else {

  // Breakdown already detected.
  // Wait until vibration becomes normal continuously.

  if (deltaAcceleration <= MOVEMENT_THRESHOLD) {

    if (normalSince == 0) {
      normalSince = millis();
    }

    if (millis() - normalSince >= AUTO_CLEAR_TIME) {

      abnormalMovement = false;
      abnormalCount = 0;
      normalSince = 0;

      Serial.println();
      Serial.println("Vibration normal for 10 seconds.");
      Serial.println("Breakdown alert CLEARED.");
      Serial.println();
    }

  } else {

    // Abnormal vibration came back.
    normalSince = 0;
  }
}
}

// =====================================================
// MOTOR FORWARD
// =====================================================

void forward() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// =====================================================
// MOTOR BACKWARD
// =====================================================

void backward() {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// =====================================================
// STOP MOTORS
// =====================================================

void stopMotors() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// =====================================================
// READ OBSTACLE AND FUEL
// =====================================================

void readSafetySensors() {

  // Obstacle sensor
  obstacleDetected =
    (digitalRead(OBSTACLE_PIN) == LOW);

  // Fuel switch
  fuelLow =
    (digitalRead(FUEL_PIN) == LOW);
}

// =====================================================
// BUZZER
// =====================================================

void updateBuzzer() {

  if (obstacleDetected ||
      fuelLow ||
      abnormalMovement) {

    digitalWrite(BUZZER_PIN, HIGH);

  } else {

    digitalWrite(BUZZER_PIN, LOW);
  }
}

// =====================================================
// MOTOR SAFETY
// =====================================================

void updateMotors() {

  if (obstacleDetected ||
      fuelLow ||
      abnormalMovement) {

    stopMotors();

  } else {

    forward();
  }
}

// =====================================================
// LCD
// =====================================================

void updateLCD() {

  lcd.clear();

  // ---------------------------------------------------
  // OBSTACLE
  // ---------------------------------------------------

  if (obstacleDetected) {

    lcd.setCursor(0, 0);

    lcd.print("OBSTACLE!");

    lcd.setCursor(0, 1);

    lcd.print("VEHICLE STOP");

    return;
  }

  // ---------------------------------------------------
  // FUEL LOW
  // ---------------------------------------------------

  if (fuelLow) {

    lcd.setCursor(0, 0);

    lcd.print("FUEL LOW!");

    lcd.setCursor(0, 1);

    lcd.print("VEHICLE STOP");

    return;
  }

  // ---------------------------------------------------
  // BREAKDOWN
  // ---------------------------------------------------

  if (abnormalMovement) {

    lcd.setCursor(0, 0);

    lcd.print("BREAKDOWN!");

    lcd.setCursor(0, 1);

    lcd.print("ASSISTANCE");

    return;
  }

  // ---------------------------------------------------
  // NORMAL
  // ---------------------------------------------------

  lcd.setCursor(0, 0);

  lcd.print("VEHICLE MOVING");

  lcd.setCursor(0, 1);

  if (gps.location.isValid()) {

    lcd.print("GPS OK");

  } else {

    lcd.print("GPS SEARCH...");
  }
}

// =====================================================
// CONNECT WIFI
// =====================================================

void connectWiFi() {

  Serial.println();
  Serial.println("Connecting to WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED &&
         attempts < 30) {

    delay(500);

    Serial.print(".");

    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WiFi CONNECTED!");

    Serial.print("IP Address: ");

    Serial.println(WiFi.localIP());

  } else {

    Serial.println("WiFi CONNECTION FAILED!");
  }
}

// =====================================================
// SEND DATA TO THINGSPEAK
// =====================================================

void uploadToThingSpeak() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi not connected.");

    return;
  }

  // ---------------------------------------------------
  // Process GPS data
  // ---------------------------------------------------

  readGPS();

  // ---------------------------------------------------
  // Convert acceleration to G
  // ---------------------------------------------------

  float accelerationX =
    ax / 16384.0;

  float accelerationY =
    ay / 16384.0;

  // ---------------------------------------------------
  // GPS variables
  // ---------------------------------------------------

  float latitude = 0.0;

  float longitude = 0.0;

  if (gps.location.isValid()) {

    latitude = gps.location.lat();

    longitude = gps.location.lng();
  }

  // ---------------------------------------------------
  // Create ThingSpeak URL
  // ---------------------------------------------------

  String url = String(THINGSPEAK_URL);

  url += "?api_key=";

  url += THINGSPEAK_API_KEY;

  // Field 1 = Acceleration X

  url += "&field1=";

  url += String(accelerationX, 4);

  // Field 2 = Acceleration Y

  url += "&field2=";

  url += String(accelerationY, 4);

  // Field 3 = Obstacle

  url += "&field3=";

  url += String(obstacleDetected ? 1 : 0);

  // Field 4 = Fuel Low

  url += "&field4=";

  url += String(fuelLow ? 1 : 0);

  // Field 5 = Latitude

  url += "&field5=";

  url += String(latitude, 6);

  // Field 6 = Longitude

  url += "&field6=";

  url += String(longitude, 6);
  // Field 7 = Breakdown Status

  url += "&field7=";

  url += String(abnormalMovement ? 1 : 0);

  // ---------------------------------------------------
  // Send request
  // ---------------------------------------------------

  Serial.println();
  Serial.println("Sending data to ThingSpeak...");

  HTTPClient http;

  http.begin(url);

  int httpCode = http.GET();

  Serial.print("ThingSpeak response: ");

  Serial.println(httpCode);

  if (httpCode == 200) {

    String response = http.getString();

    Serial.print("Entry ID: ");

    Serial.println(response);

    Serial.println("Cloud upload SUCCESS");

  } else {

    Serial.println("Cloud upload FAILED");
  }

  http.end();
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println(" VEHICLE BREAKDOWN ASSISTANCE");
  Serial.println("================================");

  // ===================================================
  // MOTOR PINS
  // ===================================================

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // ===================================================
  // BUZZER
  // ===================================================

  pinMode(BUZZER_PIN, OUTPUT);

  // ===================================================
  // OBSTACLE SENSOR
  // ===================================================

  pinMode(OBSTACLE_PIN, INPUT);

  // ===================================================
  // FUEL SWITCH
  // ===================================================

  pinMode(FUEL_PIN, INPUT_PULLUP);

  // ===================================================
  // START SAFELY
  // ===================================================

  stopMotors();

  digitalWrite(BUZZER_PIN, LOW);

  // ===================================================
  // I2C
  // ===================================================

  Wire.begin(21, 22);

  // ===================================================
  // LCD
  // ===================================================

  lcd.init();

  lcd.backlight();

  lcd.setCursor(0, 0);

  lcd.print("Vehicle System");

  lcd.setCursor(0, 1);

  lcd.print("Starting...");

  delay(2000);

  // ===================================================
  // MPU6050
  // ===================================================

  writeMPU(0x6B, 0x00);

  delay(100);

  // Accelerometer ±2G

  writeMPU(0x1C, 0x00);

  // Low-pass filter

  writeMPU(0x1A, 0x03);

  Serial.println("MPU6050 initialized.");

  // ===================================================
  // GPS
  // ===================================================

  GPSserial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX,
    GPS_TX
  );

  Serial.println("GPS initialized.");

  // ===================================================
  // WIFI
  // ===================================================

  connectWiFi();

  Serial.println();
  Serial.println("System ready!");
  Serial.println();

  lcd.clear();

  lcd.setCursor(0, 0);

  lcd.print("SYSTEM READY");

  lcd.setCursor(0, 1);

  lcd.print("VEHICLE MOVING");

  delay(2000);
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // GPS
  // ===================================================

  readGPS();

  // ===================================================
  // MPU
  // ===================================================

  readMPU();

  // ===================================================
  // OBSTACLE + FUEL
  // ===================================================

  readSafetySensors();

  // ===================================================
  // SAFETY
  // ===================================================

  updateMotors();

  updateBuzzer();

  // ===================================================
  // SERIAL MONITOR
  // ===================================================

  Serial.println("----------------------------");

  Serial.print("AX: ");

  Serial.print(ax);

  Serial.print("   AY: ");

  Serial.print(ay);

  Serial.print("   AZ: ");

  Serial.print(az);

  Serial.print("   TOTAL: ");

  Serial.print(totalAcceleration);

  Serial.print("   DELTA: ");

  Serial.println(deltaAcceleration);

  Serial.println();

  // ---------------------------------------------------
  // OBSTACLE
  // ---------------------------------------------------

  if (obstacleDetected) {

    Serial.println("OBSTACLE: DETECTED");

  } else {

    Serial.println("OBSTACLE: CLEAR");
  }

  // ---------------------------------------------------
  // FUEL
  // ---------------------------------------------------

  if (fuelLow) {

    Serial.println("FUEL: LOW");

  } else {

    Serial.println("FUEL: NORMAL");
  }

  // ---------------------------------------------------
  // VEHICLE
  // ---------------------------------------------------

  if (abnormalMovement) {

    Serial.println(
      "VEHICLE: BREAKDOWN DETECTED"
    );

  } else {

    Serial.println(
      "VEHICLE: MOVING"
    );
  }

  // ===================================================
  // GPS DISPLAY
  // ===================================================

  if (gps.location.isValid()) {

    Serial.print("Latitude: ");

    Serial.println(
      gps.location.lat(),
      6
    );

    Serial.print("Longitude: ");

    Serial.println(
      gps.location.lng(),
      6
    );

    Serial.print("Satellites: ");

    Serial.println(
      gps.satellites.value()
    );

  } else {

    Serial.println(
      "GPS: Waiting for location..."
    );
  }

  // ===================================================
  // LCD
  // ===================================================

  updateLCD();

  // ===================================================
  // THINGSPEAK UPLOAD EVERY 15 SECONDS
  // ===================================================

  if (millis() - lastThingSpeakUpload >=
      THINGSPEAK_INTERVAL) {

    lastThingSpeakUpload = millis();

    uploadToThingSpeak();
  }

  // ===================================================
  // WIFI RECONNECT
  // ===================================================

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
      "WiFi disconnected."
    );

    connectWiFi();
  }

  delay(500);
}