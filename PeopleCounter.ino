#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <time.h>

//NTP 
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0;

//WIFI
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

//FIREBASE
#define API_KEY "YOUR_API_KEY"

#define DATABASE_URL \
"https://peoplecounter-c9e56-default-rtdb.europe-west1.firebasedatabase.app/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool firebaseReady = false;

//OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

//SENSORS
#define TRIG1 5
#define ECHO1 18

#define TRIG2 19
#define ECHO2 23

#define DETECTION_DISTANCE 10

//LED
#define RED_LED 25
#define GREEN_LED 26

//COUNTER
int currentCapacity = 0;
int maxCapacity = 5;

int peakOccupancy = 0;

unsigned long sampleCount = 0;
unsigned long sumPeople = 0;

unsigned long lastFirebaseMinute = 0;

//PASS DETECTION
bool s1Triggered = false;
bool s2Triggered = false;

int direction = 0;

unsigned long firstTriggerTime = 0;

#define MIN_PASS_TIME 180

//RESET
bool lastResetState = false;

// SETUP TIME

void setupTime() {
  configTime(
    gmtOffset_sec,
    daylightOffset_sec,
    ntpServer
  );
}

// DISTANCE

long measureDistance(int trig, int echo) {

  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);

  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 30000);

  if (duration == 0)
    return 999;

  return duration * 0.034 / 2;
}

// OLED

void updateOLED() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("People Counter");

  display.setCursor(0, 20);
  display.print("Current: ");
  display.println(currentCapacity);

  display.setCursor(0, 35);
  display.print("Max: ");
  display.println(maxCapacity);

  display.display();
}

// LED

void updateLED() {

  if (currentCapacity >= maxCapacity) {

    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);

  } else {

    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
  }
}

// FIREBASE SEND

void sendToFirebase() {

  if (!firebaseReady || !Firebase.ready())
    return;

  // Peak
  if (currentCapacity > peakOccupancy) {
    peakOccupancy = currentCapacity;
  }

  // Average
  sampleCount++;
  sumPeople += currentCapacity;

  float averageOccupancy =
    (float)sumPeople / sampleCount;

  // Basic values
  Firebase.RTDB.setInt(
    &fbdo,
    "/peopleCounter/current",
    currentCapacity
  );

  Firebase.RTDB.setInt(
    &fbdo,
    "/peopleCounter/max",
    maxCapacity
  );

  Firebase.RTDB.setInt(
    &fbdo,
    "/peopleCounter/peak",
    peakOccupancy
  );

  Firebase.RTDB.setFloat(
    &fbdo,
    "/peopleCounter/average",
    averageOccupancy
  );

  // History
  struct tm timeinfo;

  if (getLocalTime(&timeinfo)) {

    unsigned long currentMinute =
      millis() / 60000;

    if (currentMinute != lastFirebaseMinute) {

      lastFirebaseMinute = currentMinute;

      char timeKey[6];

      strftime(
        timeKey,
        sizeof(timeKey),
        "%H:%M",
        &timeinfo
      );

      String path =
        "/peopleCounter/history/" +
        String(timeKey);

      Firebase.RTDB.setInt(
        &fbdo,
        path.c_str(),
        currentCapacity
      );
    }
  }
}

// RESET FROM FIREBASE

void checkResetFromFirebase() {

  if (!Firebase.ready())
    return;

  if (Firebase.RTDB.getBool(
      &fbdo,
      "/peopleCounter/resetCounter")) {

    bool resetState = fbdo.boolData();

    if (resetState && !lastResetState) {

      currentCapacity = 0;
      peakOccupancy = 0;

      sumPeople = 0;
      sampleCount = 0;

      updateOLED();
      updateLED();

      Firebase.RTDB.setInt(
        &fbdo,
        "/peopleCounter/current",
        0
      );

      Firebase.RTDB.setInt(
        &fbdo,
        "/peopleCounter/peak",
        0
      );

      Firebase.RTDB.setFloat(
        &fbdo,
        "/peopleCounter/average",
        0
      );

      Firebase.RTDB.deleteNode(
        &fbdo,
        "/peopleCounter/history"
      );

      Firebase.RTDB.setBool(
        &fbdo,
        "/peopleCounter/resetCounter",
        false
      );
    }

    lastResetState = resetState;
  }
}

// READ MAX

void readMaxFromFirebase() {

  if (!Firebase.ready())
    return;

  if (Firebase.RTDB.getInt(
      &fbdo,
      "/peopleCounter/max")) {

    int newMax = fbdo.intData();

    if (newMax != maxCapacity &&
        newMax > 0) {

      maxCapacity = newMax;

      updateOLED();
      updateLED();
    }
  }
}

// RESTART ESP

void checkRestartFromFirebase() {

  if (Firebase.RTDB.getBool(
      &fbdo,
      "/peopleCounter/restartESP")) {

    if (fbdo.boolData()) {

      Firebase.RTDB.setBool(
        &fbdo,
        "/peopleCounter/restartESP",
        false
      );

      ESP.restart();
    }
  }
}

// SETUP

void setup() {

  Serial.begin(115200);

  // Sensor pins
  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);

  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  // LED pins
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);

  // OLED
  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C)) {

    Serial.println("OLED error");

    while (1);
  }

  updateOLED();
  updateLED();

  // WiFi
  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");

  
  setupTime();

  // Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(
        &config,
        &auth,
        "",
        "")) {

    firebaseReady = true;

    Serial.println("Firebase auth OK");

  } else {

    Serial.println(
      config.signer
      .signupError
      .message
      .c_str()
    );
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

// LOOP

void loop() {

  long d1 =
    measureDistance(TRIG1, ECHO1);

  long d2 =
    measureDistance(TRIG2, ECHO2);

  bool s1 = d1 < DETECTION_DISTANCE;
  bool s2 = d2 < DETECTION_DISTANCE;

  
  if (s1 &&
      !s1Triggered &&
      !s2Triggered) {

    s1Triggered = true;

    direction = 1;

    firstTriggerTime = millis();

    Serial.println("ENTERING");
  }

  if (s2 &&
      !s2Triggered &&
      !s1Triggered) {

    s2Triggered = true;

    direction = -1;

    firstTriggerTime = millis();

    Serial.println("EXITING");
  }

  
  if (s1 && s2Triggered)
    s1Triggered = true;

  if (s2 && s1Triggered)
    s2Triggered = true;

  
  if (!s1 &&
      !s2 &&
      s1Triggered &&
      s2Triggered) {

    unsigned long passTime =
      millis() - firstTriggerTime;

    if (passTime > MIN_PASS_TIME) {

      
      if (direction == 1 &&
          currentCapacity < maxCapacity) {

        currentCapacity++;

        Serial.println("ENTER");
      }

      
      if (direction == -1 &&
          currentCapacity > 0) {

        currentCapacity--;

        Serial.println("EXIT");
      }

      updateOLED();
      updateLED();

      sendToFirebase();

    } else {

      Serial.println(
        "Ignored fast movement"
      );
    }

    s1Triggered = false;
    s2Triggered = false;

    direction = 0;

    firstTriggerTime = 0;

    delay(250);
  }

  checkResetFromFirebase();
  readMaxFromFirebase();
  checkRestartFromFirebase();

  delay(50);
}
