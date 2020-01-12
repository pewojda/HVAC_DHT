#include <RTClib.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include "DHT.h"
#include <SPI.h>
#include <SD.h>

DHT dht(2, DHT22);
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
RTC_DS1307 rtc;

float heat_count = 0.0;
float chill_count = 0.0;
float non_count = 0.0;

float heat_last_count = 0.0;
float chill_last_count = 0.0;
float non_last_count = 0.0;

float heat_last = 0.0;
float chill_last = 0.0;
float non_last = 0.0;

float humidity = 0.0;
float temperature = 0.0;

int heat = 0;
int chill = 0;
int non = 0;

DateTime time;

String mainfilename = "";
String lastfilename = "";

unsigned int prevmillis = 0;

bool created = false;


//Settings
//float temps[24][2]; //Temperature;Var
unsigned int refresh = 2000; //min 2000
int time_zone = 1;
float offset = 0.0;
int mode = 1; //1-Heater 0-Chiller
int pin = 14; //A0 high on low

float temps[24][2] = {
  {17.0, 1.0}, //0:00
  {17.0, 1.0}, //1:00
  {17.0, 1.0}, //2:00
  {17.0, 1.0}, //3:00
  {17.0, 1.0}, //4:00
  {20.0, 0.2}, //5:00
  {20.0, 0.2}, //6:00
  {20.0, 0.2}, //7:00
  {16.0, 1.0}, //8:00
  {16.0, 1.0}, //9:00
  {16.0, 1.0}, //10:00
  {16.0, 1.0}, //11:00
  {16.0, 1.0}, //12:00
  {16.0, 1.0}, //13:00
  {16.0, 1.0}, //14:00
  {16.0, 1.0}, //15:00
  {16.0, 1.0}, //16:00
  {18.0, 0.5}, //17:00
  {18.0, 0.5}, //18:00
  {18.0, 0.5}, //19:00
  {18.0, 0.5}, //20:00
  {19.0, 0.5}, //21:00
  {19.0, 0.5}, //22:00
  {19.0, 0.5}, //23:00
};


String zero_app(String s) {
  if (s == 0) return "00";
  else if (s.length() < 2) return 0 + s;
  else return s;
}

void makeDirs() {
  String year_t = String(time.year());
  year_t.remove(0, 2);
  String date_str1 = String(time.day()) + "_" + String(time.month()) + "_" + year_t;
  String date_str2 = String(time.hour()) + "_" + String(time.minute());
  String dir = date_str1 + "/" + date_str2;
  if (!SD.exists(dir)) {
    SD.mkdir(dir);
    mainfilename = dir + "/LOG.CSV";
    lastfilename = dir + "/RUNS.CSV";
  }
}

float getTemp() {
  return temps[time.hour()][0];
}

float getVar() {
  return temps[time.hour()][1];
}

void error(String dev) {
  digitalWrite(pin, HIGH);
  lcd.clear();
  lcd.print(dev);
  lcd.print(" ERROR");
  while (1);
}

void updateLast() {
  File lastfile = SD.open(lastfilename, FILE_WRITE);
  if (lastfile) {
    lastfile.print(heat_last);
    lastfile.print(";");
    lastfile.print(non_last);
    lastfile.print(";");
    lastfile.println(chill_last);
    lastfile.close();
  }
  else {
    error("LAST");
  }
}

void updatetime() {
  time = rtc.now();
  if (time.unixtime() > pow(2, 31)) {
    error("RTC READ");
  }
  time = time + TimeSpan(time_zone * 3600);
}

void setup() {
  pinMode(pin, OUTPUT);
  pinMode(10, OUTPUT);
  digitalWrite(pin, HIGH);

  Serial.begin(9600);
  lcd.begin(16, 2); //RS-9,E-8,D4-7,D5-6,D6-5,D7-4
  dht.begin();
  if (!rtc.begin()) { //SCL-A5 , SDA-A4
    error("RTC INIT");
  }
  if (!rtc.isrunning()) {
    error("RTC RUN");
  }
  if (!SD.begin(10)) { //CS-10, CLK-13, MISO-12, MOSI-11
    error("SD");
  }

  updatetime();
  makeDirs();
}

void loop() {
  if (millis() - prevmillis >= refresh) {
    prevmillis = millis();

    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      error("DHT");
    }

    temperature += offset;
    updatetime();

    if (time.hour() == 0) { //New file on day
      if (!created) {
        makeDirs();
        created = true;
      }
    }
    else {
      created = false;
    }

    File mainfile = SD.open(mainfilename, FILE_WRITE);
    if (mainfile) {
      mainfile.print(time.year());
      mainfile.print("-");
      mainfile.print(zero_app(String(time.month())));
      mainfile.print("-");
      mainfile.print(zero_app(String(time.day())));
      mainfile.print(";");
      mainfile.print(zero_app(String(time.hour())));
      mainfile.print(":");
      mainfile.print(zero_app(String(time.minute())));
      mainfile.print(":");
      mainfile.print(zero_app(String(time.second())));
      mainfile.print(";");
      mainfile.print(humidity);
      mainfile.print(";");
      mainfile.print(temperature);
      mainfile.print(";");
      mainfile.print(getTemp());
      mainfile.print(";");
      mainfile.print(heat_count);
      mainfile.print(";");
      mainfile.print(non_count);
      mainfile.print(";");
      mainfile.println(chill_count);
      mainfile.close();
    }
    else {
      error("LOG");
    }

    lcd.begin(16, 2);
    lcd.clear();
    lcd.print(String(humidity, 1));
    lcd.print("% ");
    lcd.print(zero_app(String(time.hour())));
    lcd.print(":");
    lcd.print(zero_app(String(time.minute())));
    lcd.print(":");
    lcd.print(zero_app(String(time.second())));
    if (heat || chill) {
      lcd.print(" I");
    }
    else lcd.print(" O");
    lcd.setCursor(0, 2);
    lcd.print(String(temperature, 1));
    lcd.print("C ");
    lcd.print(String(getTemp(), 1));
    lcd.print("C ");
    lcd.print(String(getVar(), 1));
    lcd.print("C ");


    if (digitalRead(pin) == LOW) {
      if (mode) {
        if (temperature >= getTemp() + getVar()) {
          heat = 0;
          chill = 0;
          non = 1;
        }
        else {
          heat = 1;
          chill = 0;
          non = 0;
        }
      }
      else {
        if (temperature <= getTemp() - getVar()) {
          heat = 0;
          chill = 0;
          non = 1;
        }
        else {
          heat = 0;
          chill = 1;
          non = 0;
        }
      }
    }

    if (digitalRead(pin) == HIGH) {
      if (mode) {
        if (temperature <= getTemp() - getVar()) {
          heat = 1;
          chill = 0;
          non = 0;
        }
        else {
          heat = 0;
          chill = 0;
          non = 1;
        }
      }
      else {
        if (temperature >= getTemp() + getVar()) {
          heat = 0;
          chill = 1;
          non = 0;
        }
        else {
          heat = 0;
          chill = 0;
          non = 1;
        }
      }
    }


    if (heat == 0 && heat_last_count > 0) {
      heat_last = heat_last_count;
      heat_last_count = 0;
      updateLast();
    }
    else {
      heat_last_count += heat * refresh / 1000.0;
    }

    if (chill == 0 && chill_last_count > 0) {
      chill_last = chill_last_count;
      chill_last_count = 0;
      updateLast();
    }
    else {
      chill_last_count += chill * refresh / 1000.0;
    }

    if (non == 0 && non_last_count > 0) {
      non_last = non_last_count;
      non_last_count = 0;
      updateLast();
    }
    else {
      non_last_count += non * refresh / 1000.0;
    }


    heat_count += heat * refresh / 1000.0;
    chill_count += chill * refresh / 1000.0;
    non_count += non * refresh / 1000.0;


    if (mode) {
      digitalWrite(pin, !heat);
    }
    else {
      digitalWrite(pin, !chill);
    }

  }
}
