void gpsEncode() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
}

int getCurrentSatellitesValue() {
  gpsEncode();

  return (gps.satellites.isValid() ? gps.satellites.value() : -1);
}

long getCurrentConnectionState() {
  gpsEncode();

  if (gps.charsProcessed() < 10) {
    return ERROR;
  } else if (gps.speed.isValid() && gps.time.isValid() && gps.date.isValid()
             && gps.date.day() != 0 && gps.date.month() != 0 && getCurrentSatellitesValue() > 0) {
    return CONNECTED;
  } else {
    return CONNECTING;
  }
}

int getCurrentSpeed() {
  gpsEncode();

  if (gps.speed.isValid() && getCurrentConnectionState() == CONNECTED) {
    int _currentSpeed = round(gps.speed.kmph());

    if (_currentSpeed < 3) {
      _currentSpeed = 0;
    }

    if (_currentSpeed > maxSpeed) {
      maxSpeed = _currentSpeed;
    }

    return _currentSpeed;
  }

  return -1;
}

int getCurrentSpeedForArrow() {
  gpsEncode();

  int _currentSpeed = getCurrentSpeed();
  if (_currentSpeed != 0) {
    if (_currentSpeed >= 260) {
      return 196;
    } else if (_currentSpeed >= 20) {
      return map(_currentSpeed, 20, 260, 0, 196);
    } else if (_currentSpeed >= 3) {
      return 340 + _currentSpeed;
    }
  }

  return 343;
}

void avgSpeedAndDistanceCalculate() {
  if (avgSpeedAndDistanceTimer.isReady()) {
    static unsigned long speedSum = 0;
    static unsigned long speedCounter = 0;

    int _currentSpeed = getCurrentSpeed();

    if (_currentSpeed > 0) {
      speedSum += _currentSpeed;
      speedCounter++;
      avgSpeed = speedSum / speedCounter;

      if (gps.location.isValid() && getCurrentConnectionState() == CONNECTED) {
        static double prevLat = gps.location.lat(), prevLon = gps.location.lng();
        float distanceBetween =
          TinyGPSPlus::distanceBetween(gps.location.lat(), gps.location.lng(), prevLat, prevLon) / 1000.0f;
        prevLat = gps.location.lat();
        prevLon = gps.location.lng();

        currentDistance += distanceBetween;
        currentOdometerVal += distanceBetween;
      }
    }
  }
}

float getCurrentOdometerVal() {
  return initialOdometerVal + currentOdometerVal;
}

void tripTimeCalculate() {
  if (tripTimer.isReady()) {
    tripTime++;
  }
}

String getTripTime() {
  return str(hour((time_t)tripTime)) + ":" + str(minute((time_t)tripTime)) + ":" + str(second((time_t)tripTime));
}

void timeUpdate() {
  gpsEncode();

  if (gps.time.isValid() && gps.date.isValid() && gps.date.day() != 0 && gps.date.month() != 0) {
    TimeElements tm;
    tm.Second = gps.time.second();
    tm.Minute = gps.time.minute();
    tm.Hour = gps.time.hour();
    tm.Day = gps.date.day();
    tm.Month = gps.date.month();
    tm.Year = gps.date.year() - 1970;
    setTime(timeZone.toLocal(makeTime(tm)));
    isTimeUpdated = true;
  }
}

String getCurrentTime() {
  return (isTimeUpdated ? str(hour()) + ":" + str(minute()) : "--:--");
}

String str(int num) {
  String str = (String)num;

  return (str.length() == 1 ? "0" + str : str);
}
