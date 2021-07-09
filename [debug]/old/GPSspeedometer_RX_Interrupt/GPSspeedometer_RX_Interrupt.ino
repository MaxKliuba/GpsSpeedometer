#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include <LowPower.h>

#define INTERRUPT 2
#define DS18B20_PIN 3
#define TX 4
#define RX 5

OneWire oneWire(DS18B20_PIN);
DallasTemperature DS18B20(&oneWire);
SoftwareSerial gpsSerial(RX, TX);

const int SLEEP_TIME = 1 * 60; // час у секундах
char ch;
String inData = "";

bool sleep = false, update = false;
int page = 0;
int speedCar = 0, speedAngle = 0;
int hourTrip = 0, minuteTrip = 0, secondTrip = 0, timerSleep;
long timerMillisTrip = 0, timerTrip = 0;
double allDistance = 0.7, distance = 0;
int satellite = 0;
double temperature = 0;
int day = 0, month = 0, year = 0;
int hour = 0, minute = 0, second = 0;

void setup()
{
  Serial.begin(9600);
  DS18B20.begin();
  gpsSerial.begin(9600);

  pinMode(INTERRUPT, INPUT_PULLUP);  
  NextionWritePage("page0"); 
}

void loop() 
{
  NextionRead();

  if(!sleep)
  {
    if(page == 0)
    {
      printSpeed();
      printTime();
      printDate();
      printTemp();
    }
    else if(page == 1)
    {
      printAllDistance();
      printDistance();
      printLocation();
      printSatellite();
    }

    printTripTime();

    if(update)
      update = false;

    if(timerSleep >= SLEEP_TIME)
    {
      NextionWrite("sleep", 1);
      delay(100);
      Sleep();
    }
  }
}

void analyseInData()
{
  if(inData.indexOf("reset") >= 0)
    allDistance = 0;
  else if(inData.indexOf("page0") >= 0)
  {
    page = 0;
    update = true;
  }
  else if(inData.indexOf("page1") >= 0)
  {
    page = 1;
    update = true;
  }
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
  //if(hour != getHour() || minute != getMinute() || second != getSecond() || update)
  {
    hour = 10; minute = 51; second = 7;
    NextionWrite("page0.time.txt", printDigits(hour) + ":" + printDigits(minute) + ":" + printDigits(second));
  }
}

void printDate()
{
  //if(day != getDay() || month != getMonth() || year != getYear() || update)
  {
    day = 25; month = 10; year = 2019;
    NextionWrite("page0.date.txt", printDigits(day) + "." + printDigits(month) + "." + printDigits(year));
  }
}

void printTemp()
{
  //DS18B20.requestTemperatures();
  
  if(temperature !=  DS18B20.getTempCByIndex(0) || update)
  {
    //temperature = DS18B20.getTempCByIndex(0);
    NextionWrite("page0.temp.txt", temperature, "", 1);
  }
}

void printAllDistance()
{
  //allDistance = 0.643;
  NextionWrite("page1.allDistance.txt", allDistance, " km", 1);
}

void printDistance()
{
  distance = 0.44;
  NextionWrite("page1.distance.txt", distance, " km", 1);
}

void printTripTime()
{
  if(millis() - timerMillisTrip >= 1000 || millis() - timerMillisTrip < 0 || update)
  {
    timerMillisTrip = millis();
    timerTrip++;

    hourTrip = timerTrip / (60 * 60);
    minuteTrip = (timerTrip - (hourTrip * 60 * 60))/ 60;
    secondTrip = timerTrip - (hourTrip * 60 * 60) - (minuteTrip * 60);

    if(page == 1)
      NextionWrite("page1.tripTime.txt", printDigits(hourTrip) + ":" + printDigits(minuteTrip) + ":" + printDigits(secondTrip));

    //if(speedCar == 0)
      timerSleep++;
    //else
    //  timerSleep = 0;
  }
}

void printLocation()
{
  NextionWrite("page1.location.txt", "-30.9408, 151.7184");
}

void printSatellite()
{
  //if(satellite != getSatelite() || update)
  {
    satellite = 8; //getSatelite();
    NextionWrite("page1.satellite.txt", (String)satellite);
  }
}

void NextionRead()
{
  while(Serial.available() > 0)
  {   
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

void NextionWritePage(String page)
{
  Serial.print("page " + page);
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

void Awake()
{
  if(sleep)
  {
    timerSleep = 0;
    timerTrip = 0;
    timerMillisTrip = millis();
    Serial.begin(9600);
    sleep = false;
    update = true;
    detachInterrupt(0);
    page = 0;
  }
}

void Sleep()
{
  Serial.end();
  attachInterrupt(0, Awake, CHANGE);
  sleep = true;
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
