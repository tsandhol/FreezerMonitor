// This #include statement was automatically added by the Particle IDE.
#include "LiquidCrystal.h"

// This #include statement was automatically added by the Particle IDE.
#include "OneWire.h"

// This #include statement was automatically added by the Particle IDE.
#include "spark-dallas-temperature.h"

#define STARTING_TEMP 0.0042
double tempF = STARTING_TEMP;
//double warnThreshold = 10;
//double alarmThreshold = 32;
double warnThreshold = 77;
double alarmThreshold = 80;

double lowTemp = 1000;
double highTemp = -1000;

bool inWarnState = false;
bool inAlarmState = false;
bool alarmSounding = false;

unsigned long alarmSilenceDuration = 20*60*1000; // 20 minutes

unsigned long alarmSilencedTime = millis() - alarmSilenceDuration;
String alarmState = "";

bool _shouldHandleButton = false;


#define SpeakerPin D0

#define SensorPin A5

#define REDLITE D1
#define GREENLITE D2
#define BLUELITE D3

#define LCD_RS A0
#define LCD_ENABLE A1
#define LCD_d4 A2
#define LCD_d5 D4
#define LCD_d6 D5
#define LCD_d7 D6

#define ButtonPin A4


DallasTemperature dallas(new OneWire(SensorPin));

LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_d4, LCD_d5, LCD_d6, LCD_d7);

void setup() {


    Particle.variable("temp", tempF);
    Particle.variable("alarmst", alarmState);
    dallas.begin();
    lcd.begin(16,2);
    lcd.clear();
    lcd.print("initializing...");
    pinMode(REDLITE, OUTPUT);
    pinMode(GREENLITE, OUTPUT);
    pinMode(BLUELITE, OUTPUT);

    setBacklight(0, 0, 255);

    pinMode(ButtonPin, INPUT_PULLUP);
    attachInterrupt(ButtonPin, buttonPress, CHANGE); // was: RISING

}





void loop() {
    getTemperature();
    updateDisplay();
    checkForAlarms();
    handleButton();
    operateSpeaker();
    handleTimeSync();
}


#define TEMP_CHECK_MILLIS (5*1000)
unsigned long lastTempCheck = millis() - TEMP_CHECK_MILLIS;
void getTemperature(){

    if(millis() - lastTempCheck > TEMP_CHECK_MILLIS){
        dallas.requestTemperatures();
        float t = dallas.getTempFByIndex(0);
        if(t > -50 && t < 120){

            tempF = round(t * 10)/10;
        }

        if(tempF != STARTING_TEMP){
            if(lowTemp > tempF){
                lowTemp = tempF;
            }
            if(highTemp < tempF){
                highTemp = tempF;
            }
        }
        lastTempCheck = millis();
    }
}

int brightness = 100;
void setBacklight(uint8_t r, uint8_t g, uint8_t b) {
  // normalize the red LED - its brighter than the rest!
  r = map(r, 0, 255, 0, 100);
  g = map(g, 0, 255, 0, 150);

  r = map(r, 0, 255, 0, brightness);
  g = map(g, 0, 255, 0, brightness);
  b = map(b, 0, 255, 0, brightness);

  // common anode so invert!
  r = map(r, 0, 255, 255, 0);
  g = map(g, 0, 255, 255, 0);
  b = map(b, 0, 255, 255, 0);

  analogWrite(REDLITE, r);
  analogWrite(GREENLITE, g);
  analogWrite(BLUELITE, b);
}
double displayedTempF = 100;

void updateDisplay(){

    if(displayedTempF != tempF){
        lcd.clear();
        lcd.print("     ");
        String sf(tempF, 1);
        lcd.print(sf);
        //lcd.write(byte(0));
        lcd.print((char)223);
        lcd.print("F");
        displayedTempF = tempF;

    }

    lcd.setCursor(0,1);
    String ht(highTemp, 1);
    String lt(lowTemp, 1);

    lcd.print("lo:");
    lcd.print(lt);
    lcd.print("  hi:");
    lcd.print(ht);

     if(inAlarmState == true){
        setBacklight(255, 0, 0);
       // lcd.print("   !! ALARM !!");
    } else if(inWarnState == true){
        setBacklight(255, 154, 0);
      //  lcd.print("  !! WARNING !!");
    } else{
        setBacklight(0, 0, 255);
      //  lcd.print("  TEMP NORMAL");
    }


}


void checkForAlarms(){
    if(tempF >= alarmThreshold){
        if(!inAlarmState){
            soundAlarm();
        }
        inAlarmState = true;
        inWarnState = false;
      //  alarmState = "inAlarmState";
    } else if(tempF >= warnThreshold){
        if(!inWarnState){
            triggerWarning();
        }
        inWarnState = true;
        inAlarmState = false;
     //   alarmState = "inWarnState";
    } else{
        inAlarmState = false;
        inWarnState = false;
     //   alarmState = "inNormalState";
    }

    if(inAlarmState == false){
        alarmSilencedTime = millis() - alarmSilenceDuration;
    }
    if(inAlarmState == false && alarmSounding == true){
        silenceAlarm();
    }
}

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
#define TIME_CHECK_MILLIS (50*1000)
unsigned long lastSync = millis();
unsigned long lastTimeCheck = millis();
void handleTimeSync(){
    if (millis() - lastSync > ONE_DAY_MILLIS) {
        // Request time synchronization from the Particle Cloud
        Particle.syncTime();
        lastSync = millis();
    }
    if(millis() - lastTimeCheck > TIME_CHECK_MILLIS){
        if(Time.hour() == 0 && Time.minute() == 0){
            resetLimits();
        }
        lastTimeCheck = millis();
    }
}

void resetLimits(){
    lowTemp = 1000;
    highTemp = -1000;
    lastTimeCheck = millis() - TIME_CHECK_MILLIS;
}

void handleButton(){
    //TODO: actually check for button press
    bool buttonPressed = _shouldHandleButton;
    if(buttonPressed){
        if(alarmSounding){
            silenceAlarm();
        } else {
            resetLimits();
        }
        _shouldHandleButton = false;
    }
}

void triggerWarning(){
    String temp = String(tempF);
    Particle.publish("temp-warning", temp, 60, PRIVATE);
}
void soundAlarm(){
    String temp = String(tempF);
    Particle.publish("temp-alarm", temp, 60, PRIVATE);
}
bool isAlarmSilenced(){
    return (millis() - alarmSilencedTime) < alarmSilenceDuration;
}
void silenceAlarm(){
    alarmSilencedTime = millis();
    noTone(SpeakerPin);
}

void alarmSong(){
    // notes in the melody:
//int melody[] = {1908,2551};
int melody[] = {2551,3098};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {8,8 };

 // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 2; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000/noteDurations[thisNote];
    tone(SpeakerPin, melody[thisNote],noteDuration);


    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(SpeakerPin);
  }
}



void operateSpeaker(){
    alarmSounding = inAlarmState && !isAlarmSilenced();

    if(alarmSounding){
        alarmSong();
    }
}



unsigned long _lastInterruptTime = millis();
#define INTERRUPT_DEBOUNCE_INTERVAL_MILLIS (20)

unsigned long _buttonPressedTime = 0;
#define BUTTON_PRESS_MINIMUM_MILLIS (150)
bool shouldProcessButtonPress(){
    if(_shouldHandleButton){
        return false;
    }
    return true;
}

void buttonPress(){



    if(((millis() - _lastInterruptTime) > INTERRUPT_DEBOUNCE_INTERVAL_MILLIS) && shouldProcessButtonPress()) {

        // just set a bool and get on with life to shorten the code in this interrupt handler.
        _shouldHandleButton = true;
        _buttonPressedTime = millis();
    }
    _lastInterruptTime = millis();
}
