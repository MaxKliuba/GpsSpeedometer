int dataIn = 0, dataOut = 0;
int v = 2;
 
void setup() 
{
Serial.begin(9600);
}

void loop()
{
 if(v == 1)
 {
  if(dataIn <= 260)
  {
    if(dataIn < 19)
      dataOut = map(dataIn, 0, 18, 346, 360);
    else if(dataIn == 19)
      dataOut = 1;
    else if(dataIn >= 20 && dataIn <= 260)
      dataOut = map(dataIn, 19, 260, 3, 194);
  }
  else
    v = 0;
 }
 else if(v == 2)
 {
   if(dataIn <= 100)
     dataOut = map(dataIn, 0, 100, 5, 93);
   else
     v = 0;
 }

 if(v != 0)
 {
   Serial.println((String)dataIn + " -> " + dataOut);
   dataIn++;
 }
}
