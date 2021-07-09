#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include <LowPower.h>
#include <avr/interrupt.h>

#define DS18B20_PIN 3
#define TX 4
#define RX 5

OneWire oneWire(DS18B20_PIN);
DallasTemperature DS18B20(&oneWire);
SoftwareSerial gpsSerial(RX, TX);

char ch;
String inData = "";
bool sleep = false;

int page = 0;
int speedCar = 0, speedAngle = 0;
int hourTrip = 0, minuteTrip = 0, secondTrip = 0, lastSecond = 0, second = 0;
long timerTrip = 0;
double allDistance = 0, distance = 0;

int satellite = 0;                //
double temperature = 0;           //
int day = 0, month = 0, year = 0; //
int hour = 0, minute = 0;         //

void setup()
{
  //pinMode(3, OUTPUT);
  //digitalWrite(3, HIGH);
  
  Serial.begin(9600);
  DS18B20.begin();
  gpsSerial.begin(9600);
}

void loop() 
{
  NextionRead();

  if(!sleep)
  {
    if(page == 0)
    {
      printSpeed();
      //printTime();
      //printDate();
      printTemp();
    }
    else if(page == 1)
    {
      printAllDistance();
      printDistance();
      printTripTime();
      printLocation();
      printSatellite();
    }
  
    if(second != lastSecond)
    {
      second = lastSecond;
      timerTrip++;
    }
  }
  else
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF); 
}

void analyseInData()
{
  if(inData.indexOf("reset") >= 0)
    digitalWrite(13, !digitalRead(13));
  else if(inData.indexOf("page0") >= 0)
    page = 0;
  else if(inData.indexOf("page1") >= 0)
    page = 1;
  else if(inData.indexOf("sleep") >= 0)
    Sleep();
}

void printSpeed()
{
  if(speedCar < 19)
    speedAngle = map(speedCar, 0, 18, 346, 360);
  else if(speedCar == 19)
    speedAngle = 1;
  else if(speedCar >= 20 && speedCar <= 260)
    speedAngle = map(speedCar, 19, 260, 2, 194);

  NextionWrite("page0.speedImage.val", speedAngle);
  NextionWrite("page0.speed.txt", (String)speedCar);

  if(speedCar < 260)
    speedCar++;
  else
  {
    speedCar = 0;
  }
}

void printTime()
{
  hour = 10; minute = 51; second = 7;
  NextionWrite("page0.time.txt", printDigits(hour) + ":" + printDigits(minute) + ":" + printDigits(second));
}

void printDate()
{
  day = 25; month = 10; year = 2019;
  NextionWrite("page0.date.txt", printDigits(day) + "." + printDigits(month) + "." + printDigits(year));
}

void printTemp()
{
  DS18B20.requestTemperatures();
  
  if(temperature !=  DS18B20.getTempCByIndex(0))
  {
    temperature = DS18B20.getTempCByIndex(0);
    NextionWrite("page0.temp.txt", temperature, "", 1);
  }
}

void printAllDistance()
{
  allDistance = 0.643;
  NextionWrite("page1.allDistance.txt", allDistance, " km", 1);
}

void printDistance()
{
  distance = 0.44;
  NextionWrite("page1.distance.txt", distance, " km", 1);
}

void printTripTime()
{
  //timerTrip = millis() / 1000;
  hourTrip = timerTrip / (60 * 60);
  minuteTrip = (timerTrip - (hourTrip * 60 * 60))/ 60;
  secondTrip = timerTrip - (hourTrip * 60 * 60) - (minuteTrip * 60);

  NextionWrite("page1.tripTime.txt", (String)hourTrip + printDigits(minuteTrip) + printDigits(secondTrip));
}

void printLocation()
{
  NextionWrite("page1.location.txt", "-30.9408, 151.7184");
}

void printSatellite()
{
  satellite = 8;
  NextionWrite("page1.satellite.txt", (String)satellite);
}

void NextionRead()
{
  while(Serial.available() > 0)
  {
    if(sleep)
    {
      sleep = false;
      Serial.print("page page0");
      Serial.write(0xFF);
      Serial.write(0xFF);
      Serial.write(0xFF);
      return;
    }
    
    ch = Serial.read();
    inData += ch;
    
    if(ch == 0x0A)
    {
      analyseInData();
      inData = "";
      return;
    }
  }
}

void NextionWrite(String element, String data)
{
  Serial.print(element + "=" + "\"" + data+ "\"");
  Serial.write(0xFF);
  Serial.write(0xFF);
  Serial.write(0xFF);
}

void NextionWrite(String element, double data, String unit, int round)
{
  Serial.print(element + "=" + "\"");
  Serial.print(data, round);
  Serial.print(unit + "\"");
  Serial.write(0xFF);
  Serial.write(0xFF);
  Serial.write(0xFF);
}

void NextionWrite(String element, int data)
{
  Serial.print(element + "=" + data);
  Serial.write(0xFF);
  Serial.write(0xFF);
  Serial.write(0xFF);
}

String printDigits(int digits)
{
  if(digits < 10)
    return "0" + (String)digits;
  else
    return (String)digits;
}

void Sleep()
{
  sleep = true; 
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
}
