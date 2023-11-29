#include "WString.h"
// RC_POWER_SWITCH.h
#ifndef RC_POWER_SWITCH_h
#define RC_POWER_SWITCH_h
#include <RCSwitch.h>
#include <Arduino.h>

class RC_POWER_SWITCH {
  private:
    RCSwitch rcswitch = RCSwitch();
    int pin = 0;
    int triggOnTimeHour = 0;
    int triggOnTimeMinute = 0;
    int triggOffTimeHour = 0;
    int triggOffTimeMinute = 0;
    float todaySunHours = 0;
    //This is the middle of the day (13:15)
    const float dayMedian=13.25;
    unsigned long switchoff_code;
    unsigned long switchon_code;
    unsigned char switchState;
    
    int sunHours[12] = {0, 0, 6, 8, 10, 13, 13, 11, 10, 8, 2, 0};
    int monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  public:
    RC_POWER_SWITCH(int pin, unsigned long switchoff_code, unsigned long switchon_code);
    void turnON();
    void turnOFF();
    int getState();
    int LastDay(int, int);
    unsigned long getSwitchoffCode();
    unsigned long getSwitchonCode();
    bool checkIfTriggerOnTime(char *);
    bool checkIfTriggerOffTime(char *);
    void updateTodaySunHours(char *);
    void calcAndSetTriggerTimes();
    void printDebug(char *, char *);
    String getTriggerTimesString();
};

#endif