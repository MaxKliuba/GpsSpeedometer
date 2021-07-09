#include <EEPROM.h>

#define INIT_KEY 10
#define INIT_KEY_EEPROM_ADDRESS 15
#define ODOMETER_EEPROM_ADDRESS 20

float initialOdometerVal = 0.0f;
float currentOdometerVal = 2.2f;

void setup() {
  Serial.begin(9600);
  delay(5000);
  Serial.println("[START]...");
  EEPROM.get(ODOMETER_EEPROM_ADDRESS, initialOdometerVal);
  Serial.println("[initialOdometerVal]: " + (String)initialOdometerVal);
//  EEPROM.put(ODOMETER_EEPROM_ADDRESS, getCurrentOdometerVal());
//  EEPROM.get(ODOMETER_EEPROM_ADDRESS, initialOdometerVal);
//  Serial.println("[initialOdometerVal]: " + (String)initialOdometerVal);
}

void loop() {
  /* Empty loop */
}

float getCurrentOdometerVal() {
  return initialOdometerVal + currentOdometerVal;
}
