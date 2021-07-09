#include <OneWire.h>
#include <DallasTemperature.h>

#define DS18B20_PIN 2

OneWire oneWire(DS18B20_PIN);
DallasTemperature DS18B20(&oneWire);

void setup() {
  Serial.begin(9600);
  DS18B20.begin();
}

void loop() {
  DS18B20.requestTemperatures();
  Serial.println(DS18B20.getTempCByIndex(0));
}
