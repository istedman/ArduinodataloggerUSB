//-------USB DATA-LOGGER VDIP1 VARIABLE DECLERATION-------
//USB data-logger VDIP1 serial connection
//SoftwareSerial mySerial(6, 11);
SoftwareSerial mySerial(5, 4);  

#define RTCADDRESS 0x68
#define TEMPSENSORADDR 0x49

// Function prototypes

float CalcAdcRes();
void readTime();
float readTempSens();
unsigned long dateStampForUSB();
int writeDataToStick();
int time_of_day();

// These constants won't change.  They're used to give names
// to the pins used:
const int analogIn0 = A0;  // 20V range
const int analogIn1 = A1;  // 20V range
const int analogIn2 = A2;  // 5V range
const int analogIn3 = A3;  // 5V range

#define V20InputScale 4.3 //0.23
#define V10InputScale 2 //0.5
#define V5InputScale  1 
#define CurrentInputScale 1
#define VERSION 0.2
//#define DEBUG 1 // If defined, user serial port for debug
//#define DEBUGMIN 1
//#define NOPWRSAVE

//Defines for manipulating time from RTC
#define SECONDS 0
#define MINUTES 1
#define HOURS 2
#define WEEKDAY 3  
#define DAY 4
#define MONTH 5
#define YEAR 6
  
// #defines to index this array
#define MONTHCAL 0
#define SUNRISEHOUR 1
#define SUNRISEMIN 2
#define SUNSETHOUR 3
#define SUNSETMIN 4

const char Daylight_Calendar [12][5] PROGMEM ={
//   Month, Rise_hour, Rise_min, SetHour, Set_Min
//  Times are for London and times for the 15th of the month http://www.sunsettimes.co.uk/
    1,   7,     58,   16,     21,
    2,   7,     14,   17,     16,
    3,   6,     14,   18,     9,
    4,   6,     5,    19,     57,
    5,   5,     9,    20,     46,
    6,   4,     43,   21,     19,
    7,   5,     1,    21,     11,
    8,   5,     46,   20,     23,
    9,   6,     35,   19,     15,
    10,  7,     25,   18,     7,
    11,  7,     19,   16,     11,
    12,  8,     00,   15,     52};

enum day_night {day,night} night_day;

const int LEDPin = 13;
const int PWRCtrl = 2; // MOSFET drive for VDIP

const char signMessage[] PROGMEM  = {"Starting datalogger program Ver "};
const char createdMessage[] PROGMEM = {"Compiled on "};
const char sampleMessage[] PROGMEM = {"Sample Interval is every "};
const char startDateMessage[] PROGMEM = {"Startup date is "};

//Number of characters to be written to USB pen
int noOfChars;

#define INLENGTH 30
#define INTERMINATOR 13
