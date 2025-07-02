// Arduino nano
// Главный модуль управления микроволновой печи марки "BEKO"

#include "Display_ML01AS.h"
#include "Clock.h"
#include "Inputs.h"

// Описание пинов
#define LATCH_PIN    A1  // STCP на 74HC595
#define CLOCK_PIN    A2  // SHCP на 74HC595
#define DATA_PIN     A0  // DS на 74HC595
#define SYM_1_PIN    13  // Два выхода с МК на катоды дисплея напрямую
#define SYM_2_PIN    A5
#define DOOR         A3  // Дверца микроволновой печи
#define ENCODER_CLK  11
#define ENCODER_DT   10
#define BUTTONS_ROW1 8   // Столбцы для матрицы кнопок (пины)
#define BUTTONS_ROW2 9
#define BUTTONS_COL1 6   // Строки для матрицы кнопок (пины)
#define BUTTONS_COL2 5
#define BUTTONS_COL3 4
#define BUTTONS_COL4 3

#define MAGNETRON_RELAY_PIN 12  // Выходы - магнетрон 12 и тарелка с вентилятором в паре 7
#define TABLE_RELAY_PIN     7

#define BUZZER_PIN  2    //  Пин зуммера
#define BUZZER_FREQ 2000 //  Частота зуммера

// Машина состояний
enum State {
  STATE_IDLE,
  STATE_SET_HOUR,
  STATE_SET_MINUTE,
  STATE_DEFROST_SETUP,
  STATE_DEFROST_RUNNING,
  STATE_WARM_UP_SETUP,
  STATE_WARM_UP_RUNNING,
  STATE_POWER_SETUP,
  STATE_PAUSED,
  STATE_DONE
};

const unsigned long inactivityTimeout = 4000;        //  4 секунды отсутствия активности (для режима установки часов и мощности)
const unsigned long buttonsDebounce = 180;           //  Антидребезг - кнопки в миллисекундах
const unsigned long encoderDebounce = 230;           //  Антидребезг - энкодер в миллисекундах
const unsigned long stateDoneDuration = 3500;        //  Продолжительность режима "Задание выполнено"
const unsigned long buzzerSoundDoneDuration = 500;   //  Продолжительность одного пика зуммера для режима выполненного задания
const unsigned long buzzerSoundButtonDuration = 50;  //  Продолжительность одного пика зуммера для кнопок
const unsigned long buzzerSoundEncoderDuration = 15; //  Продолжительность одного пика зуммера для энкодера
const unsigned long buzzerSoundErrorDuration = 650;  //  Продолжительность одного пика зуммера для обозначения ошибки
const unsigned long addTimeForDefrostVal = 60;       //  Время (в секундах) которое будет добавляться или убавлятся при прокрутке энкодера в режиме разморозки
const unsigned long addTimeForWarmUpVal = 10;        //  Время (в секундах) которое будет добавляться или убавлятся при прокрутке энкодера в режиме разогрева

bool doorIsOpen = true;  // Дверца открыта (на всякий случай)
bool buzzerIsActive = false;  // Зуммер не пищит сразу
unsigned long lastActivityTime = 0;  // Различные счетчики
unsigned long lastEncoderTime = 0;
unsigned long lastButtonsTime = 0;
unsigned long buzzerSoundTime = 0;
unsigned long stateDoneTime = 0;
unsigned long buzzerSoundDuration = 0;  //  Переменная для хранения текущей продолжительности звука
uint16_t iconsOnDisplay = 0;  // Биты включения иконок на дисплее (см. Display_ML01AS.cpp для информации)
int lastButtonPressed = 0;  // Для хранения последней нажатой кнопки (чтобы избежать постоянных срабатываний функций)
int countDownTimerTime = 0;  // Рабочий таймер. В секундах
uint8_t newHour = 0;  // Для установки часов
uint8_t newMinute = 1;
uint8_t currentMagnetronPowerMode = MAGNETRON_HIGH;   // Текущий режим работы магнетрона. Все режимы работы в библиотеке "Clock.h"
uint8_t selectedMagnetronPowerMode = MAGNETRON_HIGH;  // Выбираемый в настройках мощности режим магнетрона (по кнопке POWER)

ClockTime currentTime;
InputsStatus inputStatus;
State currentState = STATE_IDLE;
State lastState = STATE_IDLE;



void setup() {
  //Serial.begin(9600);  // Если нужна отладка
  //Serial.println();

  pinMode(DOOR, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
  displayInit(LATCH_PIN, CLOCK_PIN, DATA_PIN, SYM_1_PIN, SYM_2_PIN);
  initEncoder(ENCODER_CLK, ENCODER_DT);
  initClockTimer(MAGNETRON_RELAY_PIN, TABLE_RELAY_PIN);
  initButtons(BUTTONS_ROW1 , BUTTONS_ROW2, BUTTONS_COL1, BUTTONS_COL2, BUTTONS_COL3, BUTTONS_COL4);
}



void loop() {
  // Сначала нужно обязательно проверить дверцу
  checkMicrowaveDoor();

  processDoneMode();  // Проверка закончены ли задания

  readInputs();  // Далее читаем кнопки и энкодер

  processingCooking();

  displayProcessing();  

  processingBuzzer();
  
  // Проверяем таймаут (отсутствие активности энкодера или кнопок) для режима установки часов и для режима установки мощности
  if (millis() - lastActivityTime >= inactivityTimeout) {
    if (currentState == STATE_SET_HOUR || currentState == STATE_SET_MINUTE || currentState == STATE_POWER_SETUP) {
      currentState = STATE_IDLE;
      setTime(newHour, newMinute);
    }
  }
}



void displayProcessing() {
  // Выбор того что будет показывать дисплей
  switch (currentState) {
    case STATE_IDLE:
      showClock();
      break;
    case STATE_SET_HOUR:
      showSetHour();
      break;
    case STATE_SET_MINUTE:
      showSetMinute();
      break;
    case STATE_DEFROST_SETUP:
      showDefrostSetup();
      break;
    case STATE_WARM_UP_SETUP:
      showWarmUpSetup();
      break;
    case STATE_WARM_UP_RUNNING:
      showWarmUpRunning();
      break;
    case STATE_DEFROST_RUNNING:
      showDefrostRunning();
      break;
    case STATE_PAUSED:
      showPause();
      break;
    case STATE_POWER_SETUP:
      showPowerSetup();
      break;
    case STATE_DONE:
      showDoneMode();
      break;
  }

  if (doorIsOpen) {
    iconsOnDisplay |= (1 << 15);  // Включаем биты значка "замок"
  }
  else {
    iconsOnDisplay &= ~(1 << 15);  // Выключаем биты значка "замок"
  }
  displayShowSymbols(iconsOnDisplay);  // Показать иконки на дисплее
}



void showClock() {
  ClockForDisplay clock = getTimeForDisplay();

  // Показываем цифры времени
  for (int i = 0; i <= 4; i++) {
    displayDigit(clock.clockDigit[i], i + 1);
  }

  iconsOnDisplay = (1 << 2); // Для включения значка часов

  // Мигаем двоеточием каждую секунду
  if (clock.clockDigit[5] % 2 == 0) {
    iconsOnDisplay |= (1 << 13) | (1 << 14);  // Включаем биты значка двоеточия
  }
  else {    
    iconsOnDisplay &= ~((1 << 13) | (1 << 14));  // Выключаем биты значка двоеточия
  }
}



void checkMicrowaveDoor() {
  if (digitalRead(DOOR) == HIGH) {
    // Дверца открыта
    doorIsOpen = true;
    stopMagnetronAndPlate();
    if (currentState == STATE_DONE) {
      currentState = STATE_IDLE;
      buzzerStopBeep();
    }
  }
  else {
    // Дверца закрыта    
    doorIsOpen = false;
  }  
}



void readInputs() {
  int result = 0;
  inputStatus = getInputStatus();  // Собираем данные с кнопок и энкодера
  
  // Читаем энкодер
  if (millis() - lastEncoderTime > encoderDebounce) {
    result = inputStatus.encoderDelta;    
    if (result != 0) {
      lastActivityTime = millis();
      encoderProcessing(result);      
      lastEncoderTime = millis();
    }
  }
  
  // Читаем кнопки  
  if (millis() - lastButtonsTime > buttonsDebounce) {
    result = inputStatus.buttonPressed;
    if (result != 0 && lastButtonPressed != result) {
      lastButtonPressed = result;
      lastActivityTime = millis();
      buttonProcessing(result);
      lastButtonsTime = millis();
    }
    else if (result == 0) {
      lastButtonPressed = 0;
    }
  }
}



void buttonProcessing(int buttonPressed) {
  lastActivityTime = millis();  
  switch (buttonPressed) {
    case BUTTON_CLOCK:
      clockButtonProcessing();
      break;
    case BUTTON_AUTO_DEFROST:
      autoDefrostButtonProcessing();
      break;
    case BUTTON_STOP:
      buttonStopProcessing();
      break;
    case BUTTON_START:
      buttonStartProcessing();
      break;
    case BUTTON_POWER:
      buttonPowerProcessing();
      break;
  }
}



void clockButtonProcessing() {
  buzzerStartBeep(buzzerSoundButtonDuration, BUZZER_FREQ);
  currentTime = getCurrentTime();
  switch (currentState) {
    case STATE_IDLE:        
        currentState = STATE_SET_HOUR;
        newHour = currentTime.hours;
      break;
    case STATE_SET_HOUR:
        currentState = STATE_SET_MINUTE;
        newMinute = currentTime.minutes;
      break;
    case STATE_SET_MINUTE:
        currentState = STATE_IDLE;
        setTime(newHour, newMinute);
      break;
  }
}



void showSetHour() {
  displayDigit((newHour / 10) % 10, 1);
  displayDigit(newHour % 10, 2);
  iconsOnDisplay = (1 << 13) | (1 << 14) | (1 << 2);
}



void showSetMinute() {
  displayDigit((newMinute / 10) % 10, 3);
  displayDigit(newMinute % 10, 4);
  iconsOnDisplay = (1 << 13) | (1 << 14) | (1 << 2);
}



void showPause() {
  CountDownTimerForDisplay display = getCounDownTimerForDisplay();

  // Показываем цифры таймера обратного отсчета
  for (int i = 0; i < 4; i++) {
    displayDigit(display.timerDigit[i], i + 1);
  }

  iconsOnDisplay |= (1 << 5);  // Показывааем значок похожий на паузу <|>
}



void showWarmUpSetup() {
  CountDownTimerForDisplay display = getCounDownTimerForDisplay();
  for (int i = 0; i < 4; i++) {    
    displayDigit(display.timerDigit[i], i + 1);
  }

  iconsOnDisplay = (1 << 3) | (1 << 5);  // Включаем иконку разогрева и иконку в виде "<|>"  
}



void autoDefrostButtonProcessing() {
  buzzerStartBeep(buzzerSoundButtonDuration, BUZZER_FREQ);
  switch (currentState) {
    case STATE_IDLE:
      currentState = STATE_DEFROST_SETUP;
      countDownTimerTime = 0;  // Сброс счетчика
      addCountDownTimerTime(300);  // 5 Минут разморозка при первом нажатии
      break;
    case STATE_DEFROST_SETUP:
      addCountDownTimerTime(300);  // Добавляем 5 минут при каждом нажатии
      break;
  }
}



void showDefrostSetup() {
  CountDownTimerForDisplay display = getCounDownTimerForDisplay();

  // Показываем цифры таймера обратного отсчета
  for (int i = 0; i < 4; i++) {
    displayDigit(display.timerDigit[i], i + 1);
  }

  iconsOnDisplay = (1 << 8) | (1 << 5);  // Включаем иконку разморозки и иконку в виде "<|>"  
}



void addCountDownTimerTime(int time) {
  countDownTimerTime += time;
  if (countDownTimerTime > 3600) {
    countDownTimerTime = 3600;
  }
  else if (countDownTimerTime < 0) {
    countDownTimerTime = 0;
  }
  setCountDownTimer(countDownTimerTime);
}



void buttonStopProcessing() {
  buzzerStartBeep(buzzerSoundButtonDuration, BUZZER_FREQ);
  switch (currentState) {
    case STATE_DEFROST_SETUP:
      currentState = STATE_IDLE;
      iconsOnDisplay = 0;
      break;
    case STATE_SET_HOUR:
      currentState = STATE_IDLE;
      iconsOnDisplay = 0;
      break;
    case STATE_SET_MINUTE:
      currentState = STATE_IDLE;
      iconsOnDisplay = 0;
      break;
    case STATE_WARM_UP_SETUP:
      currentState = STATE_IDLE;
      iconsOnDisplay = 0;
      break;
    case STATE_WARM_UP_RUNNING:
      setCookingPause();
      break;
    case STATE_DEFROST_RUNNING:
      setCookingPause();
      break;
    case STATE_PAUSED:
      buzzerStartBeep(buzzerSoundErrorDuration, BUZZER_FREQ);
      currentState = STATE_IDLE;
      iconsOnDisplay = 0;
      break;
    case STATE_POWER_SETUP:
      currentState = STATE_IDLE;
      break;
    case STATE_DONE:
      currentState = STATE_IDLE;
      buzzerStopBeep();
      break;
  }
}



void showWarmUpRunning() {
  CountDownTimerForDisplay display = getCounDownTimerForDisplay();

  // Показываем цифры таймера обратного отсчета
  for (int i = 0; i < 4; i++) {
    displayDigit(display.timerDigit[i], i + 1);
  }

  iconsOnDisplay = (1 << 3);  // Включаем иконку разогрева  
}



void showDefrostRunning() {
  CountDownTimerForDisplay display = getCounDownTimerForDisplay();

  // Показываем цифры таймера обратного отсчета
  for (int i = 0; i < 4; i++) {
    displayDigit(display.timerDigit[i], i + 1);
  }

  iconsOnDisplay = (1 << 8);  // Включаем иконку разморозки  
}



void buttonStartProcessing() {
  buzzerStartBeep(buzzerSoundButtonDuration, BUZZER_FREQ);
  if (!doorIsOpen) {
    int time = getCountDownTimerTime();

    switch (currentState) {
      case STATE_IDLE:
        currentState = STATE_WARM_UP_RUNNING;
        countDownTimerTime = 0;
        addCountDownTimerTime(30);  // 30 секунд разогрев при первом нажатии
        currentMagnetronPowerMode = selectedMagnetronPowerMode;  // Устанавливаем режим работы магнетрона из настроек мощности
        startMagnetronAndPlate(currentMagnetronPowerMode);
        startCountDownTimer(true);
        break;
      case STATE_WARM_UP_RUNNING:
        countDownTimerTime = 0;
        time += 30;
        addCountDownTimerTime(time);  // Добавляем 30 секунд разогрев при каждом нажатии
        break;
      case STATE_DEFROST_SETUP:
        currentState = STATE_DEFROST_RUNNING;
        startCountDownTimer(true);
        currentMagnetronPowerMode = MAGNETRON_DEFROST;
        startMagnetronAndPlate(currentMagnetronPowerMode);
        break;
      case STATE_WARM_UP_SETUP:
        currentState = STATE_WARM_UP_RUNNING;
        currentMagnetronPowerMode = selectedMagnetronPowerMode;
        startMagnetronAndPlate(currentMagnetronPowerMode);
        startCountDownTimer(true);
        break;
      case STATE_PAUSED:
        resumeCooking();
        break;
      case STATE_POWER_SETUP:
        currentState = STATE_IDLE;
        break;
    }
  }  
  else {
    buzzerStartBeep(buzzerSoundErrorDuration, BUZZER_FREQ);
  }
}



void encoderProcessing(int encoderDelta) {
  buzzerStartBeep(buzzerSoundEncoderDuration, BUZZER_FREQ);  
  switch (currentState) {
    case STATE_SET_HOUR:      
      newHour = newHour + encoderDelta;
      if (newHour > 23) {
        newHour = 0;
      }
      if (newHour < 0) {
        newHour = 23;
      }
      break;
    case STATE_SET_MINUTE:      
      newMinute = newMinute + encoderDelta;
      if (newMinute > 59) {
        newMinute = 0;
      }
      if (newMinute < 0) {
        newMinute = 59;
      }
      break;
    case STATE_IDLE:
      currentState = STATE_WARM_UP_SETUP;
      countDownTimerTime = 0;
      addCountDownTimerTime(30);
      break;
    case STATE_WARM_UP_SETUP:
      if (countDownTimerTime > 0 && encoderDelta > 0) {
        addCountDownTimerTime(encoderDelta * addTimeForWarmUpVal);
      }
      else if (countDownTimerTime > addTimeForWarmUpVal && encoderDelta < 0) {
        addCountDownTimerTime(encoderDelta * addTimeForWarmUpVal);
      }
      break;
    case STATE_DEFROST_SETUP:
      if (countDownTimerTime > 0 && encoderDelta > 0) {
        addCountDownTimerTime(encoderDelta * addTimeForDefrostVal);
      }
      else if (countDownTimerTime > addTimeForDefrostVal && encoderDelta < 0) {
        addCountDownTimerTime(encoderDelta * addTimeForDefrostVal);
      }
      break;
  }
}



void setCookingPause() {
  stopMagnetronAndPlate();
  startCountDownTimer(false);
  lastState = currentState;
  currentState = STATE_PAUSED;
}



void resumeCooking() {
  currentState = lastState;
  iconsOnDisplay &= ~(1 << 5);  // Убираем значок похожий на паузу <|>
  startMagnetronAndPlate(currentMagnetronPowerMode);
  startCountDownTimer(true);
}



void processingCooking() {
  if (doorIsOpen) {
    if (currentState == STATE_DEFROST_RUNNING || currentState == STATE_WARM_UP_RUNNING) {
      setCookingPause();
    }
  }  
}



void buttonPowerProcessing() {
  buzzerStartBeep(buzzerSoundButtonDuration, BUZZER_FREQ);
  switch (currentState) {
    case STATE_IDLE:
      currentState = STATE_POWER_SETUP;
      break;
    case STATE_POWER_SETUP:
      if (selectedMagnetronPowerMode == 0) {
        selectedMagnetronPowerMode = 1;
      }
      else {
        selectedMagnetronPowerMode = 0;
      }
      break;
  }
}



void showPowerSetup() {
  iconsOnDisplay = (1 << 12);
  if (selectedMagnetronPowerMode == 0) {
    displayChar(CHAR_H, 3);
    displayChar(CHAR_I, 4);
  }
  else {
    displayChar(CHAR_L, 3);
    displayChar(CHAR_o, 4);    
  }
}



void showDoneMode() {
  iconsOnDisplay = 0;
  displayChar(CHAR_E, 2);
  displayChar(CHAR_n, 3);
  displayChar(CHAR_d, 4);
}



int lastResult = 1;  // Переменная для хранения последнего значения вычислений в функции ниже

void processDoneMode() {
  if (getStateDone()) {  // Если задание выполнено
    clearStateDone();
    currentState = STATE_DONE;
    stateDoneTime = millis();    
  }

  if (currentState == STATE_DONE) {
    unsigned long time = millis() - stateDoneTime; 
    int result = time / buzzerSoundDoneDuration / 2;  // Так можно получить очередь из значений 0..1..2 и т.д. в зависимости от продолжительности звука зуммера    

    if (lastResult != result) {  // Сравниваем число последоваетльности и предыдущее значение последовательности (это время будет = длительность звука * 2, ипользуется для череды пиков зуммера)
      lastResult = result;
      buzzerStartBeep(buzzerSoundDoneDuration, BUZZER_FREQ);
    }
    
    if (stateDoneTime + stateDoneDuration < millis()) {
      currentState = STATE_IDLE;
      buzzerStopBeep();
    }    
  }
}



void buzzerStartBeep(int duration, int freq) {
  buzzerIsActive = true;
  buzzerSoundDuration = duration;
  tone(BUZZER_PIN, freq);
  buzzerSoundTime = millis();
}



void buzzerStopBeep() {
  noTone(BUZZER_PIN);
}



void processingBuzzer() {
  if (buzzerIsActive) {
    if (buzzerSoundTime + buzzerSoundDuration < millis()) {
      buzzerIsActive = false;
      buzzerStopBeep();
    }
  }
}