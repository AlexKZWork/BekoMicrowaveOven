// Библиотека часов реального времени микроволновой печи марки "BEKO"
#include "Arduino.h"
#include "Clock.h"

// Массив хранящий работу магнетрона каждую секунду (хранится 10 секунд для каждого режима, 0 индекс макс мощность, 1 - средняя и 2 индекс разморозка)
const uint8_t magnetronPowerScheme[3][10] = {
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},  // 1 = HIGH, 0 = LOW
  {1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
  {1, 1, 1, 0, 0, 0, 0, 0, 0, 0}   // Читать следует так - первые 3 секунды магнетрон включен остальные 7 отключен (Разморозка)
};

static ClockTime currentTime = {0, 1, 0};  // Одна минута первого ночи (время при инициализации микроконтроллера)
static volatile bool countDownTimerActive = false;  // Рабочий таймер отклчен
static volatile bool stateDoneRequired = false;  // Требуется запуск состояния машины STATE_DONE
static volatile int countDownTimer = 0;  // Таймер с обратным отсчетом в секундах (для режимов разморозки и разогрева)
static volatile uint8_t _magnetronRelayPin;
static volatile uint8_t _tableRelayPin;
static volatile uint8_t _magnetronPowerMode = MAGNETRON_HIGH;



void initClockTimer(uint8_t magnetronRelayPin, uint8_t tableRelayPin) {
  _magnetronRelayPin = magnetronRelayPin; 
  _tableRelayPin = tableRelayPin;
  pinMode(_magnetronRelayPin, OUTPUT);
  pinMode(_tableRelayPin, OUTPUT);
  digitalWrite(_magnetronRelayPin, LOW);
  digitalWrite(_tableRelayPin, LOW);
  
  cli(); // Отключаем прерывания

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 15624;            // Сравнение каждые 1 сек (при 16 МГц / 1024)
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12) | (1 << CS10); // Предделитель 1024
  TIMSK1 |= (1 << OCIE1A);  // Разрешить прерывание по совпадению

  sei(); // Включаем прерывания
}



ISR(TIMER1_COMPA_vect) {
  currentTime.seconds++;
  if (currentTime.seconds >= 60) {
    currentTime.seconds = 0;
    currentTime.minutes++;
    if (currentTime.minutes >= 60) {
      currentTime.minutes = 0;
      currentTime.hours++;
      if (currentTime.hours >= 24) {
        currentTime.hours = 0;
      }
    }
  }

  if ((countDownTimerActive) && (countDownTimer > 0)) {
    countDownTimer--;
    if (countDownTimer == 0) {
      countDownTimerActive = false;
      stateDoneRequired = true;
      stopMagnetronAndPlate();
    }
    else {
      int index = (countDownTimer % 10) *(-1) + 9;  // Берем последнюю цифру счетчика и разворачиваем (для массива режимов работы магнетрона)
      digitalWrite(_magnetronRelayPin, magnetronPowerScheme[_magnetronPowerMode][index]);
    }
  }
}



ClockForDisplay getTimeForDisplay() {
  ClockForDisplay time;

  noInterrupts();
  time.clockDigit[0] = (currentTime.hours / 10) % 10;
  time.clockDigit[1] = currentTime.hours % 10;  
  time.clockDigit[2] = (currentTime.minutes / 10) % 10;
  time.clockDigit[3] = currentTime.minutes % 10;
  time.clockDigit[4] = (currentTime.seconds / 10) % 10;
  time.clockDigit[5] = currentTime.seconds % 10;
  interrupts();

  return time;
}



ClockTime getCurrentTime() {
  ClockTime time;

  noInterrupts();
  time.hours = currentTime.hours;
  time.minutes = currentTime.minutes;
  time.seconds = currentTime.seconds;
  interrupts();

  return time;
}



void setTime(uint8_t newHours, uint8_t newMinutes) {
  noInterrupts();
  currentTime.hours = newHours;
  currentTime.minutes = newMinutes;
  interrupts();
}



CountDownTimerForDisplay getCounDownTimerForDisplay() {
  CountDownTimerForDisplay ct;
  
  noInterrupts();
  int minutes = countDownTimer / 60;
  int seconds = countDownTimer % 60;
  ct.timerDigit[0] = (minutes / 10) % 10;
  ct.timerDigit[1] = minutes % 10;
  ct.timerDigit[2] = (seconds / 10) % 10;
  ct.timerDigit[3] = seconds % 10;
  interrupts();

  return ct;
}



void setCountDownTimer(int seconds) {
  countDownTimer = seconds;
}



void startCountDownTimer(bool start) {
  countDownTimerActive = start;
  if (start) {
    clearStateDone();
  }
}



void clearStateDone() {
  stateDoneRequired = false;
}



bool getStateDone() {
  return stateDoneRequired;
}



int getCountDownTimerTime() {
  return countDownTimer;
}

void stopMagnetronAndPlate() {
  digitalWrite(_magnetronRelayPin, LOW);
  digitalWrite(_tableRelayPin, LOW);
}



void startMagnetronAndPlate(uint8_t magnetronPowerMode) {
  _magnetronPowerMode = magnetronPowerMode;
  digitalWrite(_tableRelayPin, HIGH);
}