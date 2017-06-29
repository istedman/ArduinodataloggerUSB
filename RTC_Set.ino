// A simple program to set the Real Time clock of the DS1338
// I2C Clock and SRAM device.
// Version 1.0
// 17th March 2013
// Description
//
// A program to set the Real Time Clock on the USB datalogger. Runs as an Arduino Sketch, prompts the user, via
// the Serial Monitor, to enter the correct data. Then proceeds to print the current time every second.
//
// The DS1338 contains the Real Time Clock & date in 8 registers
// Register Map note BCD is used
// Address    Description
// 00         seconds (00-59)
// 01         Minutes (00-59)
// 02         Hours Note (00-23) Bit 6 = 0 sets 24 hour clock, bit 4 10 hour
// 03         Day of the week (1-7)
// 04         Date (1-31)
// 05         Month (1-12)
// 06         Year  (00-99)
// 07         Control (set to 03h to disable square wave output)
//
// To set the clock, will send 8 bytes, in the order shown above, to the device
// We need to get the time from the user before setting the clock
//
// What should have been a simple program took a few hours to write, partly due to inexperience with the Aduino programming
// language. Reading from the serial port, I had to add code to wait for a byte, if statements shot straight through. Silly
// mistake to make in retrospect, I was expecting a scanf/gets type function.
// The serial.flush command does not flush the receive buffer anymore. There were occasional stray bytes and newlines in 
// the input stream. This caused the serial.parseint function to return 0 which was unexpected but makes sense.
// The delay(40), while(Serial.available()>0) Serial.read(); code was to flush the receive 64 byte buffer before the next user
// input.
// The DS1307/DS1338 require BCD numbers, have to convert the integer to BCD or it all goes wrong.
// To print the BCD in the serial terminal, you print HEX.
//
// HOW to use this.
// Upload the Sketch and open the serial monitor
// Enter the data as requested.
// Press reset to start again.


#include <Wire.h>

#define RTCADDRESS 0x68 // All Arduino I2C addresses are in 7 bit format. Datasheet is $D0/D1
unsigned char TimeData[8];
unsigned char index=0;
int dataread;
unsigned char var=0;

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
return ( (val/10*16) + (val%10) );
}

void setup()
{
 Wire.begin();  // Init the I2C interface. 
 Serial.begin(9600);
 pinMode(2,OUTPUT);
 digitalWrite(2,HIGH);  // Switch on Datalogger shield
 Serial.println("Real time clock setup program");
 Serial.print("Enter the current Month (1-12)");

  
    while(!Serial.available()) ; // Wait until a byte is received notice the ; after the while()
    dataread=Serial.parseInt();
    Serial.println(dataread);  // Print the user value and a newline character
    TimeData[5] = decToBcd(dataread);
    delay(40);
    while(Serial.available()>0) Serial.read(); // Flush the input buffer

    Serial.print("Enter the current day (1-31)");
    while(!Serial.available()) ; // Wait until a byte is received notice the ; after the while()
    dataread=Serial.parseInt();
    Serial.println(dataread);  // Print the user value and a newline character
    TimeData[4] = decToBcd(dataread);
    delay(40);
    while(Serial.available()>0) Serial.read(); // Flush the input buffer
    
   
    Serial.print("Enter the current year (0-99)");
    while(!Serial.available()) ; // Wait until a byte is received notice the ; after the while()
    dataread=Serial.parseInt();
    Serial.println(dataread);  // Print the user value and a newline character
    TimeData[6] = decToBcd(dataread);
    delay(40);
    while(Serial.available()>0) Serial.read(); // Flush the input buffer
  
    Serial.print("Enter the current hour (0-23)");
    while(!Serial.available()) ; // Wait until a byte is received notice the ; after the while()
    dataread=Serial.parseInt();
    Serial.println(dataread);  // Print the user value and a newline character
    TimeData[2] = decToBcd(dataread);
    delay(40);
    while(Serial.available()>0) Serial.read(); // Flush the input buffer

    Serial.print("Enter the current minutes (0-59)");
    while(!Serial.available()) ; // Wait until a byte is received notice the ; after the while()
    dataread=Serial.parseInt();
    Serial.println(dataread);  // Print the user value and a newline character
    TimeData[1] = decToBcd(dataread);
    delay(40);
    while(Serial.available()>0) Serial.read(); // Flush the input buffer  
    
    Serial.print("Enter the day of the week (1=Sun, 2=Mon, 3=Tue, 4=Wed, 5=Thu, 6=Fri, 7=Sat");
    while(!Serial.available()) ; // Wait until a byte is received notice the ; after the while()
    dataread=Serial.parseInt();
    Serial.println(dataread);  // Print the user value and a newline character
    TimeData[3] = decToBcd(dataread);
    delay(40);
    while(Serial.available()>0) Serial.read(); // Flush the input buffer    

// Now we could ask for the seconds or we fill it to 0.
  TimeData[0] = 0;
  TimeData[7] = 0x10; // Control word. SQWE=1, oscillator output 1 Hz.
  
  // Now write the array to the I2C device 
   Wire.beginTransmission(RTCADDRESS);
   Wire.write(0);  // select first register (seconds)
   
   for (index=0; index <7; index++)
   {
     Wire.write(TimeData[index]);
    }
   // Now end the I2C transaction
   Wire.endTransmission();
   // All done
   Serial.println("Device initialsed");  // Print the user value and a newline character
}

void loop()
{

   int idx = 0;
  unsigned int timedate[8];
  Wire.beginTransmission(RTCADDRESS); // Slave address
  Wire.write(0);               //  Select first register (seconds)
  Wire.endTransmission();
  Wire.beginTransmission(RTCADDRESS);
  Wire.requestFrom(RTCADDRESS, 7);
  // Debug code below
  #ifdef DEBUG
  Serial.println("Raw data follows");
  #endif
  while(Wire.available())    // slave may send less than requested
  { 
       timedate[idx] = Wire.read();
       #ifdef DEBUG
       Serial.println(timedate[idx],HEX);
       #endif
    idx++;
  }  
// Byte 0 = seconds
// Byte 1 = minutes
// Byte 2 = hours
// Byte 3 = Day 1-7
// Byte 4 = Day of the month
// Byte 5 = Month
// Byte 6 = Year (0-99)
  Serial.print("Date ");
  Serial.print(timedate[4],HEX);
  Serial.print("/");
  Serial.print(timedate[5],HEX);
  Serial.print("/");
  Serial.print(timedate[6],HEX);
  Serial.print(" ");
  Serial.print("Time stamp ");
  Serial.print(timedate[2],HEX);
  Serial.print(":");
  Serial.print(timedate[1],HEX);
  Serial.print(":");
  Serial.println(timedate[0],HEX);

  delay (1000);


} // End loop
