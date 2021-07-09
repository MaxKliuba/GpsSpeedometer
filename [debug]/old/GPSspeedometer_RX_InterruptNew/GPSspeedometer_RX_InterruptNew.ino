#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <DS3232RTC.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LowPower.h>

#define INTERRUPT 2
#define RX 4
#define TX 5
#define GPS_PIN 7
#define DS18B20_PIN 6

#define notConnectedPic 2
#define connectedPic 3

TinyGPSPlus gps;
SoftwareSerial gpsSerial(RX, TX);
OneWire oneWire(DS18B20_PIN);
DallasTemperature DS18B20(&oneWire);
DeviceAddress devicsAddress;

const int SLEEP_TIME = 30 * 60; // час у секундах
const int DELTA_HOUR = 2;

char ch;
String inData = "";

bool printTempNULL = false, printLocNULL = false, printSatNULL = false;
bool sleep = false, update = false, connected = false;
byte page = 0, satellite = 0;
byte  hourTrip = 0, minuteTrip = 0, secondTrip = 0, sec = 0, dayD = 0, timerMillisTrip = 0;
int speedCar = 0, lastestSpeed = 0, speedAngle = 0, timerSleep = 0;
unsigned long timerTrip = 0, delayTimer = 0, timerMillisSleep = 0, gpsChars = 0;
float temperature, distance = 0, allDistance = 0;
double lat, lng;

void setup()
{
  Serial.begin(9600);
  
  pinMode(GPS_PIN, OUTPUT);
  digitalWrite(GPS_PIN, HIGH);
  delay(50);
  gpsSerial.begin(9600);
  
  DS18B20.begin();

  pinMode(INTERRUPT, INPUT_PULLUP);

  DS18B20.getAddress(devicsAddress, 0);
  
  setSyncProvider(RTC.get);
  //setTime(13,26,30,22,12,2019); // (години, хвилини, секунди, число, місяць, рік)
  //RTC.set(now());

  NextionSetPage("page0");
}

void loop() 
{
  NextionRead();
  smartDelay(500);
  
  printSpeed();
    
  if(page == 0)
  {
    printDateTime();
    printTemp();
  }
  else if(page == 1)
  {
    printLocation();
    printSatellite();
  }

  printDateTime();

  if(update)
    update = false;
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

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (gpsSerial.available())
      gps.encode(gpsSerial.read());
  } while (millis() - start < ms);

  if(gps.location.isValid() && gps.satellites.value() > 0 && gps.charsProcessed() != gpsChars)
    {
      if(!connected)
      {
        connected = true;
        NextionWrite("page0.connection.pic", connectedPic);

        lat = gps.location.lat();
        lng = gps.location.lng();

        if((hour() != (gps.time.hour() + DELTA_HOUR) || minute() != gps.time.minute()) && gps.date.day() != 0)
        {
          if(gps.time.hour() < 24 - DELTA_HOUR)
          {
            setTime(gps.time.hour() + DELTA_HOUR, gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());      
            RTC.set(now());
          }
        } 
      }
    }
    else
    {
      if(connected)
      {
        connected = false;
        NextionWrite("page0.connection.pic", notConnectedPic);
      }
    }
}

static void updateDataFromGPS(unsigned long ms)
{
  if(millis() - delayTimer >= ms || millis() - delayTimer < 0)
  {
    delayTimer = millis();

    for(byte i = 0; i < 1000; i++)
    {
      while (gpsSerial.available())
        gps.encode(gpsSerial.read());
    }
  
    if(gps.location.isValid() && gps.satellites.value() > 0 && gps.charsProcessed() != gpsChars)
    {
      if(!connected)
      {
        connected = true;
        NextionWrite("page0.connection.pic", connectedPic);

        lat = gps.location.lat();
        lng = gps.location.lng();

        if((hour() != (gps.time.hour() + DELTA_HOUR) || minute() != gps.time.minute()) && gps.date.day() != 0)
        {
          if(gps.time.hour() < 24 - DELTA_HOUR)
          {
            setTime(gps.time.hour() + DELTA_HOUR, gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());      
            RTC.set(now());
          }
        }
      }
    }
    else
    {
      if(connected)
      {
        connected = false;
        NextionWrite("page0.connection.pic", notConnectedPic);
      }
    }
  }
}

void checkSleep()
{
  if(millis() - timerMillisSleep >= 1000 || millis() - timerMillisSleep < 0)
  {
    timerSleep++;
    timerMillisSleep = millis();

    if(timerSleep >= SLEEP_TIME)
    {
      NextionWrite("sleep", 1);
      delay(100);
      Sleep();
    }
  }
}

void printSpeed()
{
  if(connected)
  {
    if(gps.speed.isValid())
    {
      speedCar = constrain(gps.speed.kmph(), 0, 260);
  
      //if(speedCar <= 2)
      //  speedCar = 0;
    }
  }
  else
  {
    speedCar = 0;
  }

  if(speedCar == 0)
  {
     checkSleep();
  }
  else
  {
    if(timerSleep != 0)
      timerSleep = 0;
  }
  
  if(page == 0 && lastestSpeed != speedCar)
  {
    lastestSpeed = speedCar;
    
    if(speedCar < 19)
      speedAngle = map(speedCar, 0, 18, 346, 360);
    else if(speedCar == 19)
      speedAngle = 1;
    else if(speedCar >= 20 && speedCar <= 260)
      speedAngle = map(speedCar, 19, 260, 2, 194);
  
    NextionWrite("page0.speedImage.val", speedAngle);
    NextionWrite("page0.speed.txt", (String)speedCar);
  }
}

void printDateTime()
{
  if(sec != second() || update)
  {
    if(page == 0)
      NextionWrite("page0.time.txt", printDigits(hour()) + ":" + printDigits(minute()) + ":" + printDigits(second()));
      
    sec = second();

    if((dayD != day() || update) && page == 0)
    {
      NextionWrite("page0.date.txt", weekdayENG() + " " + printDigits(day()) + "." + printDigits(month()) + "." + printDigits(year()));
      dayD = day();
    }

    printTripTime();
  }
  
  //NextionWrite("page0.time.txt", "-- : -- : --");
  //NextionWrite("page0.time.txt", "--- --. --. ----");
}

String weekdayENG()
{
  switch(weekday())
  {
    case 1: return "SUN";
    case 2: return "MON";
    case 3: return "TUE";
    case 4: return "WED";
    case 5: return "THU";
    case 6: return "FRI";
    case 7: return "SAT";
  }
}

void printTemp()
{
  if(DS18B20.isConnected(devicsAddress))
  {
    DS18B20.requestTemperatures();
  
    if(temperature + 2 !=  DS18B20.getTempCByIndex(0) || update)
    {
      temperature = DS18B20.getTempCByIndex(0) - 2;
      NextionWrite("page0.temp.txt", temperature, "", 1);
    }

    if(printTempNULL)
      printTempNULL = false;
  }
  else
  {
    if(!printTempNULL)
    {
      NextionWrite("page0.temp.txt", "-. -");
      printTempNULL = true;
    }
  }
}

void printAllDistance()
{
  if(page == 1)
    NextionWrite("page1.allDistance.txt", allDistance + distance, " km", 1);
}


void printDistance()
{
  distance += (gps.distanceBetween(lat, lng, gps.location.lat(), gps.location.lng())) / 1000;

  if(page == 1)
    NextionWrite("page1.distance.txt", distance, " km", 1);

  lat = gps.location.lat();
  lng = gps.location.lng();
}

void printTripTime()
{
  //if(second() != timerMillisTrip || update)
  {
    gpsChars = gps.charsProcessed();
    
    timerMillisTrip = second();   
    timerTrip++;

    hourTrip = timerTrip / (60 * 60);
    minuteTrip = (timerTrip - (hourTrip * 60 * 60))/ 60;
    secondTrip = timerTrip - (hourTrip * 60 * 60) - (minuteTrip * 60);

    if(page == 1)
      NextionWrite("page1.tripTime.txt", printDigits(hourTrip) + ":" + printDigits(minuteTrip) + ":" + printDigits(secondTrip));

    if(speedCar > 0)
    {
      printDistance();
      printAllDistance();
    }
  }
}

void printLocation()
{
  if(connected)
  {
    NextionWriteLoc("page1.location.txt", (float)gps.location.lat(), (float)gps.location.lng(), 6);

    if(printLocNULL)
      printLocNULL = false;
  }
  else
  {
    if(!printLocNULL)
    {
      NextionWrite("page1.location.txt", "-. ----,  -. ----");
      printLocNULL = true;
    }
  }
}

void printSatellite()
{
  if(connected)
  {
    if(satellite != gps.satellites.value() || update)
    {
      satellite = gps.satellites.value();
      NextionWrite("page1.satellite.txt", (String)satellite);
    }

    if(printSatNULL)
      printSatNULL = false;
  }
  else
  {
    if(!printSatNULL)
    {
      NextionWrite("page1.satellite.txt", "--");
      printSatNULL = true;
      satellite = 0;
    }
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

void NextionWriteLoc(String element, float latLoc, float lngLoc, int round)
{
  Serial.print(element + "=" + "\"");
  Serial.print(latLoc, round);
  Serial.print("; ");
  Serial.print(lngLoc, round);
  Serial.print("\"");
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

void NextionSetPage(String page)
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
    digitalWrite(GPS_PIN, HIGH);
    gpsSerial.begin(9600);
    allDistance += distance;
    distance = 0;
    timerSleep = 0;
    timerTrip = 0;
    Serial.begin(9600);
    sleep = false;
    update = true;
    detachInterrupt(0);
    page = 0;
    connected = false;
    NextionWrite("page0.connection.pic", notConnectedPic);
    printTempNULL = false;
    printLocNULL = false;
    printSatNULL = false;
  }
}

void Sleep()
{
  gpsSerial.end();
  digitalWrite(GPS_PIN, LOW);
  Serial.end();
  attachInterrupt(0, Awake, CHANGE);
  sleep = true;
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
