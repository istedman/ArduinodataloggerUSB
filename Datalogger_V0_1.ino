/*
V0.1 Datalogger software
 No power save features, just records time, temperature, voltage 
 and current and depending on debug options, either prints to serial
 port or stores on USB stick
 
 17th August 2014
 
 */

#include <Wire.h>
#include <SoftwareSerial.h>

//-------USB DATA-LOGGER VDIP1 VARIABLE DECLERATION-------
//USB data-logger VDIP1 serial connection
//SoftwareSerial mySerial(6, 11);
SoftwareSerial mySerial(5, 4);  

// Function prototypes

float CalcAdcRes();
void readTime();
float readTempSens();
unsigned long dateStampForUSB();
int writeDataToStick();

// These constants won't change.  They're used to give names
// to the pins used:
const int analogIn0 = A0;  // 20V range
const int analogIn1 = A1;  // 20V range
const int analogIn2 = A2;  // 5V range
const int analogIn3 = A3;  // 5V range
const float isenseOffset = 0.01; // Subracted from ADC readings of current

int sensorValue[4] = {
  0,0,0,0};        // value read from the pot
float Voltage[4]={
  0.0,0.0,0.0,0.0};
float Resolution=0.0;
long Vcc=0.0;

unsigned int timedate[8];  // Used to store time and date for everyone
float temperature = 0.0;
unsigned long previousSampleInt=0;

#define V20InputScale 4.3 //0.23
#define V10InputScale 2 //0.5
#define V5InputScale  1 
#define CurrentInputScale 1
#define VERSION 0.1
//#define DEBUG 1 // If defined, user serial port for debug
//#define DEBUGMIN 1
//#define NOPWRSAVE

#ifdef DEBUGMIN
unsigned long SampleInterval=20000; // how often we want to sample, in Miliseconds
#else
unsigned long SampleInterval=300000; // how often we want to sample, in Miliseconds
#endif
//Number of characters to be written to USB pen
int noOfChars;

#define INLENGTH 30
#define INTERMINATOR 13
//Character array to store incoming characters.
char inString[INLENGTH+1];
//Number of characters recieved.
int inCount;
int responseReceived = 1;

int LEDPin = 13;
int PWRCtrl = 2; // MOSFET drive for VDIP

void setup()
{
  Serial.begin(9600);  // start serial for output
  Serial.print("Starting datalogger program Ver ");
  Serial.println(VERSION);
  Serial.print("Interval is ");
  Serial.println(SampleInterval);
  analogReference(INTERNAL);
  Vcc=CalcAdcRes();
  Resolution=(float)Vcc/1024000.00; // Store calculated ADC resolution
  // Initilise I2C for RTC and temperature sensor
  analogReference(DEFAULT); // Back to normal for the ADC
  Wire.begin();
  Wire.beginTransmission(0x49); // Slave address
  Wire.write(4);               // Control register
  Wire.write(0);              // Set 11 bit resolution on the temperature sensor
  Wire.endTransmission();
  pinMode(LEDPin,OUTPUT);
  pinMode(PWRCtrl,OUTPUT);
  //-------USB DATA-LOGGER VDIP1 VARIABLE DECLARATION------- 
  // set the data rate for the SoftSerial port
  mySerial.begin(9600);
  #ifdef NOPWRSAVE
  digitalWrite(PWRCtrl,HIGH);  // Switch on shield
  delay(5000);
  mySerial.write(13);
  mySerial.print("IPA");
  mySerial.write(13);
  delay(1500);
  #else
  digitalWrite(PWRCtrl,LOW);  // By default switch off shield
  #endif
   readTime();
#ifdef DEBUG  
  Serial.println("DEBUG is defined");
#endif
  Serial.println("Setupdone");

}
/*
CalcAdcRes() reads the internal 1.1V reference and using this scales the 0-5V range of 
 the ADC 
 */
float CalcAdcRes()
{
  long result;
  //Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV was 1126400L
  return(result);
} 

// Read the I2C RTC @ address D0/D1 or $68 in 7 bit
void readTime()
{
  int idx = 0;
  Wire.beginTransmission(0x68); // Slave address
  Wire.write(0);               //  Select first register (seconds)
  Wire.endTransmission();
  Wire.beginTransmission(0x68);
  Wire.requestFrom(0x68, 7);

  while(Wire.available())    // slave may send less than requested
  { 
    timedate[idx] = Wire.read();
    idx++;
  }  
  // Byte 0 = seconds
  // Byte 1 = minutes
  // Byte 2 = hours
  // Byte 3 = Day 1-7
  // Byte 4 = Day of the month
  // Byte 5 = Month
  // Byte 6 = Year (0-99)
}

float readTempSens()
{
  Wire.beginTransmission(0x49); // Slave address
  Wire.write(0);               //  Select Temperature register
  Wire.endTransmission();
  Wire.beginTransmission(0x49);
  Wire.requestFrom(0x49, 2);    // request 2 bytes from the thermometer, now temperature reg selected.
  unsigned int temp[4], idx;

  while(Wire.available())    // slave may send less than requested
  { 
    temp[idx] = Wire.read();
    idx++;
  }  
  // Convert to floating point
  double temp_val = 0.0;
  unsigned int temp_MSB = temp[0] & 0x7F;
  temp_val = (2.0 * temp_MSB);
  unsigned int temp_LSB = (temp[1] >> 2);
  temp_val += (0.03125 * temp_LSB); 
  return(temp_val);
}
// From http://playground.arduino.cc/Main/UsbMemory but adapted for my RTC device.
unsigned long DailyFilenameForUSB()
{
  unsigned int day = timedate[4];         //but this could make the code less logical and
  unsigned int month = timedate[5];       //more difficult to debugg  
  unsigned int year = timedate[6];  
  unsigned long datecalc=(year*65536)|(month<<8)|(day);
  return datecalc;
} 
  
unsigned long dateStampForUSB()
{
  unsigned int second, minute, hour, day, month, year, lsfilestamp;
  unsigned long filestamp;
  
  
  second=timedate[0];
  minute=timedate[1];
  hour=timedate[2];
  day=timedate[4];
  month=timedate[5];
  year=timedate[6];
  hour=hour/16* 10 + hour % 16;
  minute=minute/16 * 10 + minute % 16;
  second=(second/16 * 10 + second % 16)/2;         //usb drive only works to 2s accuracy 

  day=day/16 * 10 + day % 16;
  month=month/16 * 10 + month % 16;
  year=20 + year/16 * 10 + year % 16;              //vdip dates start 0=1980, rtc0=2000, so we need to add 20 first
  filestamp =(year<<9)|(month<<5)|(day);
  lsfilestamp = (hour<<11)|(minute<<5)|(second);
  filestamp=(filestamp <<16);
  filestamp=(filestamp)|(lsfilestamp);
  return(filestamp);
  
}
  
int writeDataToStick()
{
  /* USB output is: 
   (Day=1)/(Month=1)/(Year=1),(Hours=1):(Minutes=1),(Temperature=4), Voltage[0] (PV Cell=4), Voltage[1] (Battery=4), Voltage[2](Charge Current=4)
   Numbers represent number of characters.
   */
  unsigned long fileNumber;
  unsigned long fileDate;
  int inbyte=0;
  int bytesSent=0;
  int bytecount=0;
  int loopy=0;
  
  fileNumber=DailyFilenameForUSB();       //gets daily filename
  fileDate=dateStampForUSB();
  //Debug sanity!!!!
  mySerial.write(13);
  mySerial.print("OPW ");
  mySerial.print(fileNumber,HEX);        // Use datestamp for a unique filename.
  mySerial.print(".TXT ");   
  mySerial.print("0x");
  mySerial.print(fileDate, HEX);
  mySerial.write(13);
  delay(200);
  Check4VDIPError();

  #ifdef DEBUG
  Serial.print("OPW ");
  Serial.print(fileNumber,HEX);
  Serial.print(".TXT ");
  Serial.print(fileDate,HEX);
  Serial.write(13);
  #endif
  
  //Debug
  Serial.print("S ");
  //Let the VDIP1 know we want to write
  mySerial.print("WRF ");              
  mySerial.print(noOfChars);           
  mySerial.write(13);
  delay(200); 
  Check4VDIPError();
  // Send the data
  #ifdef DEBUG
  Serial.print("WRF ");
  Serial.print(noOfChars);
  Serial.write(13);
  #endif
  
  bytesSent=mySerial.print(timedate[4],HEX);  // Day
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print("/");
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(timedate[5],HEX);  // Month
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print("/");
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(timedate[6],HEX);  // Year
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(',');
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(timedate[2],HEX);  // Hour
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(":");
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(timedate[1],HEX);  // Minute
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(":");
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(timedate[0],HEX);  // second
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(',');
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(temperature);
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(',');
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(Voltage[0]);      // PV Cell voltage
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(',');
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(Voltage[1]);      // Battery voltage
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(',');
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(Voltage[2]);      // Charge current
  bytecount+=bytesSent;
  
  bytesSent=mySerial.print(' ');
  bytecount+=bytesSent;
  
  bytesSent=mySerial.write(10);  // line feed
  bytecount+=bytesSent;
  
  bytesSent=mySerial.write(13);  // Carriage return
  bytecount+=bytesSent;
  
  if (bytecount < noOfChars)
  {  
    bytesSent= noOfChars- bytecount;  // Work out how many spaces
    #ifdef DEBUG
    Serial.print(" Chars remaining: ");
    Serial.print(bytesSent);
    #endif
    for (loopy=0; loopy<bytesSent; loopy++)
    {
        if (loopy==bytesSent)
        {
          mySerial.println(" ");
        }
        else
        {
          mySerial.print(" ");
        }  
        
    } // End for loop
     
  }  // End if (bytecount < noOfChars)
    
  #ifdef DEBUG
  Serial.print("FINISH, wrote ");
  Serial.print(bytecount);
  #endif 
 //Close the data file.
  mySerial.write(13);
  delay(200);
  Check4VDIPError();
  mySerial.print("CLF ");
  mySerial.print(fileNumber,HEX);       
  mySerial.print(".TXT");
  mySerial.write(13);
  delay(2000);
  Check4VDIPError();
  #ifdef DEBUG
  Serial.print(" CLF ");
  Serial.print(fileNumber,HEX);
  Serial.print(".TXT");
  #endif 
  
  //Print out a letter to serial so we can see whats happening.
  Serial.println("E");

} // End function

//Catch characters being sent by the VDIP1 board so we can see whats happening
 
int Check4VDIPError()
{
   int inCount = 0;
   char inString[32];
   #ifdef DEBUG
   Serial.print(" Check4VDIP");
   #endif
   
   while (mySerial.available()) 
   {
      inString[inCount] = mySerial.read();    
      delay(2);
      if (inString [inCount] == INTERMINATOR) break;
      //inCount++ < INLENGTH; 
      inCount++; 
   }
  #ifdef DEBUG
      Serial.print(" inCount ");
      Serial.println(inCount);
 #endif
  
  inString[inCount] = 0;                     // null terminate the string
  
  //If we see a No Disk message from the VDIP1 board and are trying to write data then stop it.
  //Not actually checking for 'No Disk' but for something 7 characters long that ends with a k...
  //if (inCount==7 && writeData==1 && inString[inCount-1]=='k') 
  if (inCount==7 && inString[inCount-1]=='k') 
  {
     Serial.println("Stopped"); 
  }
  if (inCount==4 && inString[inCount-1]=='>') 
  {
      responseReceived=1;
  }
  
  //Output messages to serial.
  if (inCount !=0) {
     Serial.println(inString);
     Serial.print(" ");
     Serial.println(inCount);
  }
 
} // End Check4VDIP Errorfunction  


// Function to power up, release Reset and re-send init codes

int PwrUpVDIP(void)
{
  #ifdef DEBUGMIN
  Serial.print(" Powering up ");
  #else
  Serial.print(" PU ");
  #endif
  digitalWrite(PWRCtrl,HIGH);  // Switch on shield
  delay(5000);
  mySerial.write(13);
  mySerial.print("IPA");
  mySerial.write(13);
  delay(1500);
}

int PwrDwnVDIP(void)
{
  #ifdef DEBUGMIN
  Serial.println(" Powering down ");
  #else
  Serial.println(" PD ");
  #endif
  delay(2000);
  digitalWrite(PWRCtrl,LOW);
}
  

void loop() {
  // read the analog inputs 
  int i=0;
  unsigned long currentSampleInt =millis();
  if(currentSampleInt - previousSampleInt > SampleInterval)
  {  
    digitalWrite(LEDPin,HIGH);
    #ifndef NOPWRSAVE
    PwrUpVDIP();
    #endif
    previousSampleInt = currentSampleInt;
    sensorValue[0] = analogRead(analogIn0);            
    delay(10);  
    sensorValue[1] = analogRead(analogIn1);            
    delay(10);  
    sensorValue[2] = analogRead(analogIn2);            
    delay(10);  

    Voltage[0]= Resolution*sensorValue[0]*V20InputScale;
    Voltage[1]= Resolution*sensorValue[1]*V20InputScale;
    Voltage[2]= (Resolution*sensorValue[2]*CurrentInputScale)-isenseOffset;
    readTime();
    temperature=readTempSens();
    noOfChars=sizeof(timedate)+sizeof(Voltage)+14; /* 12 is number of commas and CR/LF at end of line */
    writeDataToStick();
    
    #ifndef NOPWRSAVE
    PwrDwnVDIP();
    #endif
    digitalWrite(LEDPin,LOW);
  } // end if 


} // End loop




