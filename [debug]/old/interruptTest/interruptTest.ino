bool ledState = false;
void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  pinMode(2, INPUT);
  attachInterrupt(0, blink, FALLING);
}

void loop() {
}

void blink()
{
  ledState = !ledState;
  digitalWrite(13, ledState);
}

