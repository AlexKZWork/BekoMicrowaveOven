// Библиотека часов реального времени и управления магнетроном микроволновой печи марки "BEKO". Заголовочный файл.

#include <stdint.h>
#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>

#define MAGNETRON_HIGH    0  // Режимы работы магнетрона
#define MAGNETRON_LOW     1
#define MAGNETRON_DEFROST 2

// Структура для возврата времени в Main (для дисплея)
struct ClockForDisplay {
  uint8_t clockDigit[6];  // Для хранения каждой цифры времени
};

// Структура для хранения реального времени
struct ClockTime {
  volatile uint8_t hours;
  volatile uint8_t minutes;
  volatile uint8_t seconds;
};

// Структура обратного отсчета для показа на дисплее (для режимов разморозки и разогрева)
struct CountDownTimerForDisplay {
  uint8_t timerDigit[4];  // Для хранения каждой цифры оставшегося времени
};



void initClockTimer(uint8_t magnetronRelayPin, uint8_t tableRelayPin);
ClockForDisplay getTimeForDisplay();
ClockTime getCurrentTime();
CountDownTimerForDisplay getCounDownTimerForDisplay();
void setTime(uint8_t newHours, uint8_t newMinutes);
void setCountDownTimer(int seconds);
void startCountDownTimer(bool start);
void clearStateDone();
bool getStateDone();
int getCountDownTimerTime();
void startMagnetronAndPlate(uint8_t magnetronPowerMode);
void stopMagnetronAndPlate();



#endif