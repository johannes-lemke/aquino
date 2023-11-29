#include <OneWire.h>
#include <DallasTemperature.h>
#include "RC_POWER_SWITCH.h"
#include "HardwareSerial.h"
#include <LiquidCrystal_I2C.h>
#include <time.h>
#include <DS3231.h>

// Data wire is conntec to the Arduino digital pin 4
#define ONE_WIRE_BUS 2
#define RC_SWITCH_PIN 3
#define LED 12
// Setup the lcd screen
LiquidCrystal_I2C lcd(0x27,16,2);

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// Create RC power switch
RC_POWER_SWITCH second_socket = RC_POWER_SWITCH(RC_SWITCH_PIN, 1394004, 1394007);

//Create the realtime clock
DS3231 rtc = DS3231(SDA, SCL);    

//LED-State
bool ledOn = false;

//Temperature of the Water by month (0=January...11=December)
float waterTempsPerMonth[] = {
  12.5,
  12.5,
  18.5,
  21,
  24,
  27,
  28,
  28,
  25,
  22,
  18.5,
  12.5  
};

//Array of all menu options
String MenuItems[] = {
  "Wassertemp",
  "Wasserwechsel am",
  "Sonne",
  "Wassertemp soll",
  "Zeit"
};

//Start with the first Menu-Option
int currentMenuIndex = 0;

//Used to reduce interval for the call of showMenu();
int tickCounter = 0;

//Count seconds to timeout display
int displayTimeoutTick = 0;

//Is the Backlight currently on
bool isBacklightOn = true;

//Used for button hold input
int pushButtonTickCounter = 0;

//Used to check if the rtc is already set to summertime. Gets overriden in setup()
bool is_already_summertime = false;

//Assumes that the water is clean right now
bool waterIsClean = true;

Time lastCleanDate;
Time nextCleanDate;

void setup(void)
{
  //Initialize rtc object
  rtc.begin();

  //Set the time when it is wrong (Only use once)
  //rtc.setDate(12, 11, 2023);
  //rtc.setTime(17, 15, 30);

  // Start serial communication for debugging purposes
  Serial.begin(9600);
  
  //Start temp measure
  sensors.begin();
  
  //Setup Buttons 
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  
  //Setup LEDs
  pinMode(4, OUTPUT);

  //Initialize the lcd screen
  lcd.begin(16,2);
  lcd.init();
  lcd.backlight();

  //Assume it was the same time(Summer-/Wintertime) when the program started as it is now.
  is_already_summertime=summertime_EU(rtc.getTime().year, rtc.getTime().mon, rtc.getTime().date, rtc.getTime().hour, 1);

  lastCleanDate = rtc.getTime();
}

void loop(void){
  //Get Date and Time from Realtime Clock
  char *currentDate = rtc.getDateStr();
  char *currentTime = rtc.getTimeStr();   

  //Fake the Date
  //currentDate = "15.04.2023";
  
  //apply the daylight savings. (Summer-/Wintertime)
  applyDaylightSavings();
  
  //Serial.println(displayTimeoutTick);
  if(displayTimeoutTick == 200){
    if (isBacklightOn){
      lcd.noBacklight();
      isBacklightOn=false;
    }
    displayTimeoutTick = 0;
  }
  
  wakeupDisplay();

  //if (isTimestamp14DaysAgo(lastCleanDate)){
  //  digitalWrite(4,HIGH);    
  //}
  checkIfCleaned();
  
  //Button on pin 5 triggers to scroll backwards in Menu.
  if(digitalRead(5)){
    scrollMenu(-1);
  }  
  //Button on pin 6 triggers to scroll forwards in Menu.  
  if(digitalRead(6)){
    scrollMenu(1);
  }
  
  //Only update LCD-Screen every 10 ticks (10 loops)
  if (tickCounter == 9 ){    
    showMenu();
    tickCounter = 0;
  }  
  
  //Update the total time the poweroutlet will be on today.
  second_socket.updateTodaySunHours(currentDate);

  //Set the times to trigger the power outlet on and off.
  second_socket.calcAndSetTriggerTimes();

  //Print debug output of the socket.
  //second_socket.printDebug(currentTime, currentDate);

  //Check if now is the time to switch the poweroutlet on or off.
  if (second_socket.checkIfTriggerOnTime(currentTime) && ! second_socket.getState()){
    Serial.println("Turn On !!");
    second_socket.turnON();
  } else if (second_socket.checkIfTriggerOffTime(currentTime) && second_socket.getState()){
    Serial.println("Turn Off !!");
    second_socket.turnOFF();
  }
  
  //Increment the tick counter. Used to reduce the interval showMenu() is called
  displayTimeoutTick++;
  tickCounter++;
  //Wait for the next loop iteration
  delay(100);
}

//Draws the menu depending on the currentMenuIndex variable
void showMenu(){
  lcd.setCursor(0,0);  
  lcd.print(MenuItems[currentMenuIndex]);  
  lcd.setCursor(0,1);
  //Serial.println(currentMenuIndex);
  switch(currentMenuIndex) {
    case 0:
      printtemp();       
      break;
    case 1:
      nextCleanDate = getNextCleanDate();
      lcd.print(String(nextCleanDate.date));
      lcd.print(".");
      lcd.print(String(nextCleanDate.mon));
      lcd.print(".");
      lcd.print(String(nextCleanDate.year));
      if(digitalRead(7) && ledOn){
        pushButtonTickCounter++;
        if (pushButtonTickCounter == 3){
          ledOn = false;
          digitalWrite(4,LOW);
          lastCleanDate=rtc.getTime();
          pushButtonTickCounter=0;
        }
      }else {
        pushButtonTickCounter=0;
      }
      //Serial.println(pushButtonTickCounter);
      break;
    case 2:
      lcd.print(second_socket.getTriggerTimesString());
      break;
    case 3:
      lcd.print(waterTempsPerMonth[rtc.getTime().mon - 1]);
      lcd.print(" ");
      lcd.print((char)223);
      lcd.print("C");
      break;
    case 4:
      lcd.setCursor(0,0);
      lcd.print(rtc.getDateStr());
      lcd.setCursor(0,1);
      lcd.print(rtc.getTimeStr());
      break;
    default:
      Serial.println("ERROR invalid value for currentMenuIndex");
  }  
}

//Prints the current temperature which is read by the tempsensor.
void printtemp(){
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  //Serial.print("Celsius temperature: ");
  // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
  //Serial.println(sensors.getTempCByIndex(0));
  lcd.print(sensors.getTempCByIndex(0));
  lcd.print(" ");
  lcd.print((char)223);
  lcd.print("C");
}

void wakeupDisplay(){
  if (digitalRead(5) || digitalRead(6) || digitalRead(7) || digitalRead(8)){
    displayTimeoutTick = 0;
    if (!isBacklightOn){
      lcd.backlight();
      isBacklightOn = true; 
    }
  }
}

void scrollMenu(int direction){  
  clearLcd();
  currentMenuIndex=mod(currentMenuIndex+direction,5);
  showMenu();
  tickCounter=9;
  delay(150);
}

void clearLcd(){
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print("                ");  
}

long long mod(long long k, long long n)
{
    return ((k %= n) < 0) ? k+n : k;
}

bool summertime_EU(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{ 
  if (month<3 || month>10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month>3 && month<10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (month==3 && (hour + 24 * day)>=(1 + tzHours + 24*(31 - (5 * year /4 + 4) % 7)) || month==10 && (hour + 24 * day)<(1 + tzHours + 24*(31 - (5 * year /4 + 1) % 7))) 
    return true; 
  else 
    return false;
}

void applyDaylightSavings(){
  bool is_summertime = summertime_EU(rtc.getTime().year, rtc.getTime().mon, rtc.getTime().date, rtc.getTime().hour, 1);
  if(is_summertime && !is_already_summertime){
    Serial.println("Now it's Summer");
    is_already_summertime = true;
    int hour = rtc.getTime().hour + 1;
    rtc.setTime(hour, rtc.getTime().min, rtc.getTime().sec);       
  } else if (!is_summertime && is_already_summertime){
    Serial.println("Now it's Winter");
    is_already_summertime = false;
    int hour = rtc.getTime().hour -1;
    rtc.setTime(hour, rtc.getTime().min, rtc.getTime().sec); 
  }
}

void checkIfCleaned(){
  if(isTimestamp14DaysAgo(lastCleanDate) && !ledOn){
      ledOn = true;
      digitalWrite(4,HIGH);
  }
}

bool isTimestamp14DaysAgo(Time timestamp)
{
    // Get the current time from the DS3231 RTC
    Time now = rtc.getTime();

    // Convert timestamp from Time to tm
    tm timestamp_tm = Time2tm(timestamp);

    // Convert now from Time to tm
    tm now_tm = Time2tm(now);

    // Convert timestamp and now to time_t values
    time_t timestamp_t = mktime(&timestamp_tm);
    time_t now_t = mktime(&now_tm);    

    // Calculate the difference in days
    double diff_seconds = difftime(now_t, timestamp_t);
    int days_since_timestamp = diff_seconds / 86400;
    // Check if the difference is greater than or equal to 14 days
    if (days_since_timestamp >= 14) {
        return true;
    } else {
        return false;
    }
}

Time getNextCleanDate() {
  // Convert the current time to a tm struct
  tm timeStruct = Time2tm(lastCleanDate);

  // Add 14 days to the day of the month
  timeStruct.tm_mday += 14;
  // Normalize the tm struct (handles cases where the day of the month goes over the limit for the current month)
  time_t timeTemp = mktime(&timeStruct);
  timeStruct = *localtime(&timeTemp);

  // Convert the tm struct back to a Time object
  Time returnTime  = tmToTime(timeStruct);
  return returnTime;
}

Time tmToTime(tm timeStruct) {
  Time time;
  time.sec = timeStruct.tm_sec;
  time.min = timeStruct.tm_min;
  time.hour = timeStruct.tm_hour;
  time.date = timeStruct.tm_mday;
  time.mon = timeStruct.tm_mon + 1; // Add 1 to convert from 0-based to 1-based month
  time.year = timeStruct.tm_year + 1900; // Add 1900 to get the actual year value
  time.dow = timeStruct.tm_wday;
  return time;
}

tm Time2tm(Time t)
{
    tm timeinfo;

    timeinfo.tm_year = t.year - 1900;          // years since 1900
    timeinfo.tm_mon = t.mon - 1;        // months since January (0-11)
    timeinfo.tm_mday = t.date;          // day of the month (1-31)
    timeinfo.tm_hour = t.hour;          // hours since midnight (0-23)
    timeinfo.tm_min = t.min;            // minutes after the hour (0-59)
    timeinfo.tm_sec = t.sec;            // seconds after the minute (0-60)
    timeinfo.tm_wday = t.dow;           // days since Sunday (0-6)
    timeinfo.tm_yday = 0;               // not used
    timeinfo.tm_isdst = 0;              // not used

    return timeinfo;
}

//This code is working for daylightsavvings although it is in my opinion the same logic as in the production code 

// #include <iostream>
// #include <chrono>
// #include <thread>

// using namespace std;

// bool is_already_summertime;

// struct timee {
//     int day;
//     int month;
//     int year;
//     int hour;
// } mytime;

// bool summertime_EU(int year, int month, int day, int hour, int tzHours)
// // European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// // input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// // return value: returns true during Daylight Saving Time, false otherwise
// { 
//   if (month<3 || month>10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
//   if (month>3 && month<10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
//   if (month==3 && (hour + 24 * day)>=(1 + tzHours + 24*(31 - (5 * year /4 + 4) % 7)) || month==10 && (hour + 24 * day)<(1 + tzHours + 24*(31 - (5 * year /4 + 1) % 7))) 
//     return true; 
//   else 
//     return false;
// }

// void applyDaylightSavings(){
//   bool is_summertime = summertime_EU(mytime.year, mytime.month, mytime.day, mytime.hour, 1);
//   if(is_summertime && !is_already_summertime){
//     cout << "Now it's Summer" << endl;
//     is_already_summertime = true;
//     int hour = mytime.hour + 1;
//     mytime.hour=hour;
//   } else if (!is_summertime && is_already_summertime){
//     cout << "Now it's Winter" << endl;
//     is_already_summertime = false;
//     int hour = mytime.hour -1;
//     mytime.hour=hour;
//   }
// }

// int main() {
//     //Just before summer
//     //mytime.day=26;
//     //mytime.month=3;
//     //mytime.year=2023;
//     //mytime.hour=0;
//     //is_already_summertime=summertime_EU(mytime.year, mytime.month, mytime.day, mytime.hour, 1);
    
//     //Just before winter
//     mytime.day=29;
//     mytime.month=10;
//     mytime.year=2023;
//     mytime.hour=0;
//     is_already_summertime=summertime_EU(mytime.year, mytime.month, mytime.day, mytime.hour, 1);


//   while (true){
//       cout << mytime.day << "." << mytime.month << "." << mytime.year << " "<< mytime.hour <<":00" << endl ;
//       std::this_thread::sleep_for(std::chrono::seconds(1));
//       applyDaylightSavings();
//       mytime.hour++;
//   }
//   // use time() with NULL argument
  

//   return 0;
// }

// // Output: 1629799688
