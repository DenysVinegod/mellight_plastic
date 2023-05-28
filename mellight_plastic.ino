/*
 * ENCODER
 * s1 -   d2
 * s2 -   d3
 * key -  d4
 * 
 * OLED
 * sda -  a4
 * scl -  a5
 * 
 * TERMISTOR
 *   --- 5V
 *  |
 * | |100 kOhm
 * | |
 *  |
 *   --- Arduino A0
 *  |
 * | |100kOhm Termistor
 * | |
 *  |
 *   --- GND
 * 
 * STEP ENGINE DRIVER
 * Step -   d6
 * Dir -    d7
 * 
 * 
 * rpm = 60
 * 
 * HEATER
 * Enable - d8
 * 
 */

#define software_version  0.1

#define _TERMISTOR_PIN A0
#define _TERMISTOR_RESISTANCE 100600  // опір парного резистора 
                                      //термістора у дільнику
#define _TERMISTOR_BETA       3950    // Коефіцієнт термістора
#define _TERMISTOR_BASE_TEMP  25      // Базова температура
#define _TERMISTOR_BASE       100000  // Базовий опір термістора
#define _BASE_DIV_RES (float)_TERMISTOR_BASE / _TERMISTOR_RESISTANCE

#define _ENGINE_STEP_PIN    6
#define _ENGINE_DIR_PIN     7
#define _ENGINE_STEPS       48

#define _HEATER_PIN         8
#define _TEMPERATURE_LIMIT  300 // Limit heater max temp C

#define _MAIN_MENU_ITEMS    3  // Main_Menu items count
#define _CUT_MENU_ITEMS     4  // Items count in cut menu
#define _STRETCH_MENU_ITEMS 6

#include <EEPROM.h>
#include <GyverOLED.h>
#include <GyverTimers.h>
#include <EncButton.h>
#include "DRV8825.h"

struct CuttingParams {
  uint16_t  engn_rpm;
  uint8_t   engn_direction;
};

struct StretchParams {
  uint16_t  engn_rpm,
            temperature;
  uint8_t   engn_direction;

};

GyverOLED <SSD1306_128x64, OLED_BUFFER> oled;
EncButton <EB_TICK, 2, 3, 4> enc;
DRV8825 stepper;

CuttingParams cutting_params; // instances of struct will be
StretchParams stretch_params;    // contain params from eeprom

uint32_t  disp_tmr, 
          measure_tmr, 
          engn_stp_time;    // engine step time (microsec)
              
int16_t   filT,             // temperature filtred
          engn_cut_rpm,     // set rpm
          engn_stretch_rpm, // set rpm
          tmprtr_dst;
    
uint8_t   pointer = 0,
          engn_cut_direction,
          engn_stretch_direction,
          allow_engn = 0,
          allow_heater = 0;

static void startEngn(uint8_t delay_millis, uint16_t rpm);
void stopEngn(uint8_t delay_millis, uint16_t rpm);

void setup() {
  oled.init();
  oled.setContrast(225);
  welcome_screen();

  // read eeprom & write to sram
  EEPROM.get(0, cutting_params);
  EEPROM.get(0+sizeof(cutting_params), stretch_params);
  (cutting_params.engn_rpm > 0) 
    ? engn_cut_rpm = cutting_params.engn_rpm
    : engn_cut_rpm = 300;
  (cutting_params.engn_direction > 0) 
    ? engn_cut_direction = 1
    : engn_cut_direction = 0;
  (stretch_params.engn_rpm > 0) 
    ? engn_stretch_rpm = stretch_params.engn_rpm
    : engn_stretch_rpm = 300;
  (stretch_params.engn_direction > 0) 
    ? engn_stretch_direction = 1
    : engn_stretch_direction = 0;
  (stretch_params.temperature > 0) 
    ? tmprtr_dst = stretch_params.temperature
    : tmprtr_dst = 0;
  
  // For controlling engine using irq by timer
  Timer1.pause();
  Timer1.enableISR(CHANNEL_A);
  stepper.begin(_ENGINE_DIR_PIN, _ENGINE_STEP_PIN);
  
  // irq while encoder turns
  attachInterrupt(0, isr, CHANGE);
  attachInterrupt(1, isr, CHANGE);

  pinMode(_HEATER_PIN, OUTPUT);
  digitalWrite(_HEATER_PIN, HIGH);

  delay(500);
}

void loop() {
  enc.tick();
  
  //Menu controll
  if (enc.right()) {
    pointer = constrain(pointer+1, 0, _MAIN_MENU_ITEMS - 1);
  } if (enc.left()) {
    pointer = constrain(pointer-1, 0, _MAIN_MENU_ITEMS - 1);
  } if (enc.isClick()) {
    switch (pointer) {
      case 0: // Cutting
        pointer = 0;
        func_cutting(); 
        break;       
      case 1: // Stretch
        pointer = 0;
        func_stretch();
        break;       
      case 2: // Info
        func_info(); 
        break;
    }
  }

  if (millis() - measure_tmr > 100) {
    updateTemperature();
    measure_tmr = millis();
  }
  
  // Work with oled display
  if (millis() - disp_tmr > 100){
    // Оновлення відображення на OLED дисплеї
    oled.clear();
    oled.setScale(2);
    oled.setCursor(40, 0);
    oled.println(F("Меню"));
    oled.setScale(1);
    oled.print(F(
      " Нарiзання\n\r"
      " Протяжка\n\r"
      " Iнфо"));
    printPointer(pointer+2);
    oled.setCursor(80, 7);
    oled.print(F("t:"));
    oled.print(filT);
    oled.print(F(" C"));
    oled.update();
    disp_tmr = millis();
  }
}

float updateTemperature() {
  float analog 
    = _BASE_DIV_RES 
      / (1023.0f / analogRead(_TERMISTOR_PIN) - 1.0);
  analog = (log(analog) / _TERMISTOR_BETA) + 1.0 
    / (_TERMISTOR_BASE_TEMP + 273.15);
  filT += ((1.0 / analog - 273.15) - filT) * 0.2;
}

void welcome_screen(){
  oled.clear();
  oled.home();
  oled.setScale(2);
  oled.setCursor(5, 0);
  oled.println(F("Let`s make"));
  oled.setCursor(20, 3);
  oled.println(F("PLASTIC!"));
  oled.setScale(1);
  oled.setCursor(0, 6);
  oled.println(F("mr.R O B O T"));
  oled.setCursor(80, 6);
  oled.print(F("v. ")); oled.println(software_version);
  oled.setCursor(0, 7);
  oled.println(F(" & Mellight"));
  oled.update();
}

void saved_screen(){
  oled.clear();
  oled.setScale(2);
  oled.setCursor(10, 3);
  oled.print(F("Збережено"));
  oled.update();
  delay(1000);
}

void isr() {
  enc.tickISR();
}

ISR(TIMER1_A){
  stepper.step();
  enc.tick();
}

void printPointer(uint8_t ptr){
  oled.setCursor(0, ptr);
  oled.print(">");
}

void func_cutting() {
  while(true){
    enc.tick();

    engn_cut_direction 
      ? stepper.setDirection(DRV8825_CLOCK_WISE) 
      : stepper.setDirection(DRV8825_COUNTERCLOCK_WISE);

    allow_engn 
      ? Timer1.resume() 
      : Timer1.pause();
  
    if (enc.right()) {
      pointer = constrain(pointer+1, 0, _CUT_MENU_ITEMS - 1);
    } else if (enc.left()) {
      pointer = constrain(pointer-1, 0, _CUT_MENU_ITEMS - 1);
    } else if (enc.isClick()) {
      switch (pointer) {
        case 0: // start engine func
          if (!allow_engn) { // If engine was stopped
            oled.setCursor(63, 2);
            oled.print("Розгiн");
            oled.update();
            startEngn(3, engn_cut_rpm);
          } else { // slow stop
            oled.setCursor(63, 2);
            oled.print("Зупинка");
            oled.update();
            stopEngn(1, engn_cut_rpm);
          }
          break;
        case 2: // save
          cutting_params.engn_rpm       = engn_cut_rpm;
          cutting_params.engn_direction = engn_cut_direction;
          EEPROM.put(0, cutting_params);
          saved_screen();
          break;
        case 3: // exit
          Timer1.pause();
          if (allow_engn) stopEngn(1, engn_cut_rpm);
          pointer = 0;
          return; 
          break;
      }
    } else if (enc.rightH()) {
      if (pointer == 1) {
        if (engn_cut_rpm < 10) engn_cut_rpm += 1;
        else {
          (engn_cut_rpm+10 > 65500) 
            ? engn_cut_rpm = 65500 
            : engn_cut_rpm += 10;
          calculate_n_setStepTime(engn_cut_rpm);
          Timer1.setPeriod(engn_stp_time);
        }
      } else if (pointer == 0) {
        if (!allow_engn) engn_cut_direction = 1;
      }
    } else if (enc.leftH()) { 
      if (pointer == 1) {
        (engn_cut_rpm-10 < 2) 
          ? engn_cut_rpm = 1 
          : engn_cut_rpm -= 10;
        calculate_n_setStepTime(engn_cut_rpm);
        Timer1.setPeriod(engn_stp_time);
      } else if (pointer == 0) {
        if (!allow_engn) engn_cut_direction = 0;
      }
    }
  
    if (millis() - measure_tmr > 100) {
      updateTemperature();
      measure_tmr = millis();
    }
  
    if (millis() - disp_tmr > 200){
      oled.clear();
      oled.setCursor(10, 0);
      oled.setScale(2);
      oled.print(F("Нарiзання\n\r"));
      oled.setScale(1);
      oled.print(F(" Двигун\n\r"
                   " Швидкiсть\n\r"
                   " Зберегти\n\r"
                   " Назад\n\r"));
      printPointer(pointer+2);
      oled.setCursor(63, 2);
      engn_cut_direction 
        ? oled.print("-> ") 
        : oled.print("<- ");
      allow_engn ? oled.print("Так") : oled.print("Нi");
      oled.setCursor(63, 3);
      oled.print(engn_cut_rpm);
      oled.print(F(" rpm"));
      oled.setCursor(80, 7);
      oled.print(F("t:"));
      oled.print(filT);
      oled.print(F(" С"));
      oled.update();
      disp_tmr = millis();
    }
  }
}

void func_stretch() {
  while(true){
    enc.tick();

    engn_stretch_direction 
      ? stepper.setDirection(DRV8825_CLOCK_WISE) 
      : stepper.setDirection(DRV8825_COUNTERCLOCK_WISE);

    allow_engn 
      ? Timer1.resume() 
      : Timer1.pause();

    checkHeatr();

    if (enc.right()) {
      pointer = constrain(pointer+1, 0, _STRETCH_MENU_ITEMS - 1);
    } if (enc.left()) {
      pointer = constrain(pointer-1, 0, _STRETCH_MENU_ITEMS - 1);
    } if (enc.isClick()) {
      switch (pointer) {
        case 0:
          if (!allow_engn) { // If engine was stopped
            oled.setCursor(63, 2);
            oled.print("Розгiн");
            oled.update();
            startEngn(3, engn_stretch_rpm);
          } else { // slow stop
            oled.setCursor(63, 2);
            oled.print("Зупинка");
            oled.update();
            stopEngn(1, engn_stretch_rpm);
          }
          break;
        case 2: // Heater enable
          allow_heater = !allow_heater;
          break;
        case 4: // save
          stretch_params.engn_rpm       = engn_stretch_rpm;
          stretch_params.engn_direction = engn_stretch_direction;
          stretch_params.temperature    = tmprtr_dst;
          EEPROM.put(0+sizeof(cutting_params), stretch_params);
          saved_screen();
          break;
        case 5:
          if (allow_engn) stopEngn(1, engn_stretch_rpm);
          pointer = 1;
          return;
          break;
      }
    } else if (enc.rightH()) {
      if (pointer == 1) {
        if (engn_stretch_rpm < 10) engn_stretch_rpm += 1;
        else {
          (engn_stretch_rpm+10 > 65500) 
            ? engn_stretch_rpm = 65500 
            : engn_stretch_rpm += 10;
          calculate_n_setStepTime(engn_stretch_rpm);
          Timer1.setPeriod(engn_stp_time);
        }
      } else if (pointer == 0) {
        if (!allow_engn) engn_stretch_direction = 1;
      } else if (pointer == 3) {
        (tmprtr_dst+10 > _TEMPERATURE_LIMIT)
          ? tmprtr_dst = _TEMPERATURE_LIMIT
          : tmprtr_dst += 1;
      }
    } else if (enc.leftH()) { 
      if (pointer == 1) {
        (engn_stretch_rpm-10 < 2) 
          ? engn_stretch_rpm = 1 
          : engn_stretch_rpm -= 10;
        calculate_n_setStepTime(engn_stretch_rpm);
        Timer1.setPeriod(engn_stp_time);
      } else if (pointer == 0) {
        if (!allow_engn) engn_stretch_direction = 0;
      } else if (pointer == 3) {
        (tmprtr_dst-10 < 1)
          ? tmprtr_dst = 0
          : tmprtr_dst -= 1;
      }
    }


    if (millis() - measure_tmr > 100) {
      updateTemperature();
      measure_tmr = millis();
    }

    if (millis() - disp_tmr > 200){
      oled.clear();
      oled.setCursor(15, 0);
      oled.setScale(2);
      oled.print(F("Протяжка\n\r"));
      oled.setScale(1);
      oled.print(F(" Двигун\n\r"
                   " Швидкiсть\n\r"
                   " Нагрiвач\n\r"
                   " Темп-ра\n\r"
                   " Зберегти\n\r"
                   " Назад\n\r"));
      printPointer(pointer+2);
      oled.setCursor(63, 2);
      engn_stretch_direction 
        ? oled.print("->  ") 
        : oled.print("<-  ");
      allow_engn ? oled.print("Так") : oled.print("Нi");
      oled.setCursor(63, 3);
      oled.print(engn_stretch_rpm);
      oled.print(F(" rpm"));
      oled.setCursor(63, 4); // Heater
      allow_heater ? oled.print("Так ") : oled.print("Нi ");
      oled.setCursor(63, 5); // Temperature
      oled.print(tmprtr_dst);
      oled.print(F(" С"));
      oled.setCursor(80, 7);
      oled.print(F("t:"));
      oled.print(filT);
      oled.print(F(" С"));
      oled.update();
      disp_tmr = millis();
    }
    
  }
}

void func_info() {
  welcome_screen();
  while(true){
    enc.tick();
    if (enc.isClick()) return;
  }
}

void calculate_n_setStepTime(int rpm) {
  engn_stp_time = 60000000 / rpm / _ENGINE_STEPS;
}

void startEngn(
  uint8_t delay_millis = 3, 
  uint16_t rpm = engn_cut_rpm) {

  int16_t tmp_rpm = rpm;
//  allow_engn = true;
  // slow boost
  if (rpm == 1) {
    calculate_n_setStepTime(rpm);
    allow_engn = true;
  } else {
    if (rpm > 100) {
      rpm = 100;
    } else {
      rpm = 0;
    }
    allow_engn = true;
    for ( ; rpm < tmp_rpm; rpm+=2) {
      enc.tick();
      if (enc.isClick()) { // emergancy stop engine while boost
        stopEngn(1, rpm);
        rpm = tmp_rpm;
        break;
      }
      calculate_n_setStepTime(rpm);
      Timer1.setPeriod(engn_stp_time);
      delay(delay_millis*5);
      }
  }
}

void stopEngn(
  uint8_t delay_millis = 1, 
  uint16_t rpm = engn_cut_rpm) {

  for (int16_t tmp_rpm = rpm ; tmp_rpm > 0; tmp_rpm--) {
    calculate_n_setStepTime(tmp_rpm);
    Timer1.setPeriod(engn_stp_time);
    delay(delay_millis);
  }
  allow_engn = false;
}

void checkHeatr() {
  if (allow_heater) {
    (filT < tmprtr_dst) 
      ? enable_heater()
      : disable_heater();
  } else disable_heater();
}

void enable_heater() {
  digitalWrite(_HEATER_PIN, LOW);
}

void disable_heater() {
  digitalWrite(_HEATER_PIN, HIGH);
}