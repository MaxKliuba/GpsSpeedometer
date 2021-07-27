#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <EasyNextionLibrary.h>
#include <GyverTimer.h>
#include <Timezone.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include <GyverPower.h>

#define PHOTORESISTOR_PIN A0

#define INTERRUPT_PIN 2
#define MOSFET_PIN 6

#define GPS_RX_PIN 5  // Blue wire
#define GPS_TX_PIN 4  // Green wire
#define GPS_BAUD 9600

#define NEXTION_RX_PIN 0  // Blue wire
#define NEXTION_TX_PIN 1  // Yellow wire
#define NEXTION_BAUD 9600

#define START_PAGE_ID 0
#define MAIN_PAGE_ID 1
#define INFO_PAGE_ID 2

#define ERROR 63488
#define CONNECTING 64770
#define CONNECTED 63422

#define INIT_KEY 10
#define INIT_KEY_EEPROM_ADDRESS 15
#define ODOMETER_EEPROM_ADDRESS 20

#define SLEEP_PERIOD_MS 60000         // 1 minutes
#define TIME_UPDATE_INTERVAL 3600000  // 1 hour

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
EasyNex nextion(Serial);

GTimer sleepTimer(MS, SLEEP_PERIOD_MS);
GTimer avgSpeedAndDistanceTimer(MS, 1000);
GTimer tripTimer(MS, 1000);
GTimer brightnessCheckTimer(MS, 1000);
GTimer timeUpdateTimer(MS, TIME_UPDATE_INTERVAL);
GTimer blinkTimer(MS, 1000);


byte timeZoneOffset = 2;
TimeChangeRule DST_Rule = { "DST", Last, Sun, Mar, 3, (timeZoneOffset + 1) * 60 };  // Daylight time = UTC + timeZoneOffset + 1 hour
TimeChangeRule STD_Rule = { "STD", Last, Sun, Oct, 4, timeZoneOffset * 60 };        // Standard time = UTC + timeZoneOffset
Timezone timeZone(DST_Rule, STD_Rule);

bool isTimeUpdated = false;
unsigned long tripTime = 0;
byte lastCurrentPageId = START_PAGE_ID;
bool newPageLoaded = true;

int maxSpeed = -1;
int avgSpeed = -1;
float currentDistance = 0.0f;
float initialOdometerVal = 0.0f;
float currentOdometerVal = 0.0f;

long lastSentConnectionState = CONNECTING;
String lastSentTime = "--:--";
String lastSentSatellitesValue = "--";

String lastSentSpeed = "--";
long lastSentSpeedArrowVal = 343;
String lastSentMaxSpeed = "--";
String lastSentAvgSpeed = "--";
String lastSentTripTime = "--:--:--";
String lastSentDistance = "--.-";
String lastSentOdometerVal = "--.-";

void (*reset)(void) = 0;

void setup() {
  pinMode(MOSFET_PIN, OUTPUT);
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  power.setSleepMode(POWERDOWN_SLEEP);
  wakeUp();
  delay(1000);
  displayPageById(MAIN_PAGE_ID);
}

void loop() {
  gpsEncode();

  // checkAndSetBrightness();
  avgSpeedAndDistanceCalculate();
  tripTimeCalculate();

  if (!isTimeUpdated || timeUpdateTimer.isReady()) {
    timeUpdate();
  }

  nextion.NextionListen();
  renderCurrentPage();

  sleepTimerCheck();
}

void renderCurrentPage() {
  if (nextion.currentPageId != lastCurrentPageId) {
    displayPageById(nextion.currentPageId);
  }

  switch (nextion.currentPageId) {
    case START_PAGE_ID:
      break;
    case MAIN_PAGE_ID:
      renderMainPage();
      break;
    case INFO_PAGE_ID:
      renderInfoPage();
      break;
  }

  if (newPageLoaded) {
    newPageLoaded = false;
  }
}

void renderMainPage() {
  if (nextion.currentPageId != MAIN_PAGE_ID) {
    displayPageById(MAIN_PAGE_ID);
  }

  displayStatusBar();
  checkAndWriteNum("spd_arrow.val", getCurrentSpeedForArrow(), lastSentSpeedArrowVal);
  checkAndWriteStr("spd_text.txt", numToString(getCurrentSpeed(), -1, "--"), lastSentSpeed);
}

void renderInfoPage() {
  if (nextion.currentPageId != INFO_PAGE_ID) {
    displayPageById(INFO_PAGE_ID);
  }

  displayStatusBar();
  checkAndWriteStr("spd_text.txt", numToString(getCurrentSpeed(), -1, "--"), lastSentSpeed);
  checkAndWriteStr("max_spd_text.txt", numToString(maxSpeed, -1, "--"), lastSentMaxSpeed);
  checkAndWriteStr("avg_spd_text.txt", numToString(avgSpeed, -1, "--"), lastSentAvgSpeed);
  checkAndWriteStr("time_trip.txt", getTripTime(), lastSentTripTime);
  checkAndWriteStr("dist_trip.txt", floatToString(roundFloat(currentDistance, 1), -1, "--.-"), lastSentDistance);
  checkAndWriteStr("odometer.txt", floatToString(roundFloat(getCurrentOdometerVal(), 1), -1, "--.-"), lastSentOdometerVal);
}

void displayStatusBar() {
  checkAndWriteStr("time.txt", getCurrentTime(), lastSentTime);
  checkAndWriteStr("sat.txt", numToString(getCurrentSatellitesValue(), -1, "--"), lastSentSatellitesValue);
  long currentConnectionState = getCurrentConnectionState();
  static bool currentConnectionStateFlag = true;
  if (currentConnectionState == CONNECTING) {
    if (blinkTimer.isReady()) {
      currentConnectionStateFlag = !currentConnectionStateFlag;
    }
    if (!currentConnectionStateFlag) {
      currentConnectionState = 0;
    }
  }
  checkAndWriteNum("indicator.pco", currentConnectionState, lastSentConnectionState);
}

void displayPageById(byte pageId) {
  nextion.currentPageId = pageId;
  lastCurrentPageId = nextion.currentPageId;
  nextion.writeStr("page " + (String)pageId);
  delay(50);
  newPageLoaded = true;
}

String numToString(long val, long invalidVal, String invalidStringVal) {
  return (val == invalidVal ? invalidStringVal : (String)val);
}

String floatToString(float val, float invalidVal, String invalidStringVal) {
  if (val == invalidVal) {
    return invalidStringVal;
  } else {
    String strVal = (String)val;

    return strVal.substring(0, strVal.length() - 1);
  }
}

float roundFloat(float val, byte accuracy) {
  float coefficient = pow(10, accuracy);

  return (float)(round(val * coefficient) / coefficient);
}

bool checkAndWriteStr(String elem, String currentVal, String &lastSentVal) {
  if (newPageLoaded || !currentVal.equals(lastSentVal)) {
    nextion.writeStr(elem, currentVal);
    lastSentVal = currentVal;

    return true;
  }

  return false;
}

bool checkAndWriteNum(String elem, long currentVal, long &lastSentVal) {
  if (newPageLoaded || currentVal != lastSentVal) {
    nextion.writeNum(elem, currentVal);
    lastSentVal = currentVal;

    return true;
  }

  return false;
}

void gpsSerialFlush() {
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
  }
}

void trigger0() {
  // To call this void send from Nextion's component's Event:  printh 23 02 54 00
  sleep();
}

void trigger1() {
  // To call this void send from Nextion's component's Event:  printh 23 02 54 01
  initialOdometerVal = 0.0f;
  currentOdometerVal = 0.0f;
  EEPROM.put(ODOMETER_EEPROM_ADDRESS, getCurrentOdometerVal());
}

void checkAndSetBrightness() {
  if (brightnessCheckTimer.isReady()) {
    setBribhtness(constrain(map(analogRead(PHOTORESISTOR_PIN), 0, 1023, 1, 10) * 10, 10, 100));
  }
}

void setBribhtness(byte brightnessVal) {
  static byte currentBrightness = 0;

  if (currentBrightness != brightnessVal) {
    if (brightnessVal > 100) {
      brightnessVal = 100;
    }

    currentBrightness = brightnessVal;
    nextion.writeNum("dim", currentBrightness);
  }
}

void sleepTimerCheck() {
  if (getCurrentSpeed() > 5) {
    sleepTimer.reset();    
  }

  if (sleepTimer.isReady()) {
    sleep();
  }
}

void sleepSensorCheck() {
  sleepTimer.reset();
}

void wakeUp() {
  detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));

  digitalWrite(MOSFET_PIN, HIGH);

  nextion.begin(NEXTION_BAUD);
  gpsSerial.begin(GPS_BAUD);
  gpsSerialFlush();

  if (EEPROM.read(INIT_KEY_EEPROM_ADDRESS) != INIT_KEY) {
    EEPROM.update(INIT_KEY_EEPROM_ADDRESS, INIT_KEY);
    EEPROM.put(ODOMETER_EEPROM_ADDRESS, getCurrentOdometerVal());
  }
  EEPROM.get(ODOMETER_EEPROM_ADDRESS, initialOdometerVal);

  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), sleepSensorCheck, CHANGE);
}

void sleep() {
  detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));

  displayPageById(START_PAGE_ID);
  delay(1000);

  gpsSerialFlush();
  gpsSerial.end();
  Serial.end();

  digitalWrite(MOSFET_PIN, LOW);

  EEPROM.put(ODOMETER_EEPROM_ADDRESS, getCurrentOdometerVal());

  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), wakeUp, CHANGE);
  power.sleep(SLEEP_FOREVER);
  // sleep...
  reset();
}