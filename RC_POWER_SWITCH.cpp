#include "WString.h"
// RC_POWER_SWITCH.cpp
#include "RC_POWER_SWITCH.h"

using namespace std;

RC_POWER_SWITCH::RC_POWER_SWITCH(int pin, unsigned long switchoff_code, unsigned long switchon_code) {
  this->pin = pin;
  this->switchoff_code = switchoff_code;
  this->switchon_code = switchon_code;
  this->rcswitch.enableTransmit(this->pin);

  //Turn everything off
  this->turnOFF();
}

void RC_POWER_SWITCH::turnON() {
  this->switchState = HIGH;
  this->rcswitch.send(switchon_code, 24);
}

void RC_POWER_SWITCH::turnOFF() {
  this->switchState = LOW;
  this->rcswitch.send(switchoff_code, 24);
}

int RC_POWER_SWITCH::getState() {
  return this->switchState;
}

unsigned long RC_POWER_SWITCH::getSwitchoffCode() {
  return this->switchoff_code;
}

unsigned long RC_POWER_SWITCH::getSwitchonCode() {
  return this->switchon_code;
}

bool RC_POWER_SWITCH::checkIfTriggerOnTime(char *timeStr){
  int currHour, currMin;
  sscanf(timeStr, "%2d:%2d",
    &currHour,
    &currMin);
  return (currHour == this->triggOnTimeHour && currMin == this->triggOnTimeMinute);
}

bool RC_POWER_SWITCH::checkIfTriggerOffTime(char *timeStr){
  int currHour, currMin;
  sscanf(timeStr, "%2d:%2d",
    &currHour,
    &currMin);
  return (currHour == this->triggOffTimeHour && currMin == this->triggOffTimeMinute);
}

void RC_POWER_SWITCH::updateTodaySunHours(char *dateStr){
  int currMonth, currMonthDay;
  sscanf(dateStr, "%2d.%2d",
    &currMonthDay,
    &currMonth);
  int prevMonth;
  int lastDayOfCurrMonth = monthDays[currMonth - 1];
  if (currMonth == 1){
    prevMonth = 11;
  } else {
    prevMonth = currMonth  - 1;
  }
  // -1 because of index being less than actual monthnumber
  float sunhoursDiff = this->sunHours[currMonth-1] - this->sunHours[prevMonth-1];
  float sunHoursTodayDecimal=float(this->sunHours[prevMonth-1]) + (sunhoursDiff * (float(currMonthDay) / float(lastDayOfCurrMonth)));

  this->todaySunHours=sunHoursTodayDecimal;
  return;
}

void RC_POWER_SWITCH::calcAndSetTriggerTimes(){   
    float onTime = (this->dayMedian - (this->todaySunHours/2));
    float offTime = (this->dayMedian + (this->todaySunHours/2));
    
    this->triggOnTimeHour = onTime;
    this->triggOffTimeHour = offTime;
    
    float onMinutesRemainder = (onTime - this->triggOnTimeHour) * 60;
    float offMinutesRemainder = (offTime - this->triggOffTimeHour) * 60;
    
    this->triggOnTimeMinute = onMinutesRemainder;
    this->triggOffTimeMinute = offMinutesRemainder;
}

String RC_POWER_SWITCH::getTriggerTimesString(){
  String triggOnTimeHour = "";
  String triggOnTimeMinute = "";
  String triggOffTimeHour = "";
  String triggOffTimeMinute = "";
  
  if(this->triggOnTimeHour < 10){
    triggOnTimeHour = "0" + String(this->triggOnTimeHour);
  } else {
    triggOnTimeHour = String(this->triggOnTimeHour);
  }
  if(this->triggOnTimeMinute  < 10){
    triggOnTimeMinute = "0" + String(this->triggOnTimeMinute);
  } else {
    triggOnTimeMinute = String(this->triggOnTimeMinute);
  }
  if(this->triggOffTimeHour < 10){
    triggOffTimeHour = "0" + String(this->triggOffTimeHour);
  } else {
    triggOffTimeHour = String(this->triggOffTimeHour);
  }
  if(this->triggOffTimeMinute < 10){
    triggOffTimeMinute = "0" + String(this->triggOffTimeMinute);
  } else {
    triggOffTimeMinute = String(this->triggOffTimeMinute);
  }
  String str = triggOnTimeHour + ":" + triggOnTimeMinute + " bis " + triggOffTimeHour + ":" + triggOffTimeMinute;
  return str;
}

void RC_POWER_SWITCH::printDebug(char *currentDate, char *currentTime){
    
    Serial.println("===========================");
    Serial.print("| [");
    Serial.print(currentDate);
    Serial.print(" - ");
    Serial.print(currentTime);
    Serial.println("] |");
    Serial.println("===========================");

    Serial.print("Sunhours today: ");
    Serial.println(this->todaySunHours);
    Serial.print("Turn on at: ");
    Serial.print(this->triggOnTimeHour);
    Serial.print(":");
    Serial.println(this->triggOnTimeMinute);
    Serial.print("Turn off at: ");    
    Serial.print(this->triggOffTimeHour);
    Serial.print(":");
    Serial.println(this->triggOffTimeMinute);
}