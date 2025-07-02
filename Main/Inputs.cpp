#include "Arduino.h"
// Библиотека опроса кнопок и энкодера микроволновой печи марки "BEKO"

#include "Inputs.h"

static int currentStateCLK;
static int lastStateCLK;
static int _pin_CLK;
static int _pin_DT;

static InputsStatus inputStatus;

static const byte numRows = 4;
static const byte numCols = 2;

static byte rowPins[numRows];  // строки (входы)
static byte colPins[numCols];  // столбцы (выходы)

static int keyMap[numRows][numCols] = {
  {1, 2},
  {3, 4},
  {5, 6},
  {7, 8}
};



void initEncoder(uint8_t pin_CLK, uint8_t pin_DT) {
  _pin_CLK = pin_CLK;
  _pin_DT = pin_DT;

  pinMode(_pin_CLK, INPUT_PULLUP);
  pinMode(_pin_DT, INPUT_PULLUP);
  lastStateCLK = digitalRead(_pin_CLK);
}



void initButtons(uint8_t colPin1, uint8_t colPin2, uint8_t rowPin1, uint8_t rowPin2, uint8_t rowPin3, uint8_t rowPin4) {
  colPins[0] = colPin1;
  colPins[1] = colPin2;

  rowPins[0] = rowPin1;
  rowPins[1] = rowPin2;
  rowPins[2] = rowPin3;
  rowPins[3] = rowPin4;

  // Настроим строки как входы с подтяжкой
  for (byte i = 0; i < numRows; i++) {
    pinMode(rowPins[i], INPUT_PULLUP);
  }

  // Столбцы как выходы и держим в HIGH
  for (byte j = 0; j < numCols; j++) {
    pinMode(colPins[j], OUTPUT);
    digitalWrite(colPins[j], HIGH);
  }
}



void readEncoder() {
  currentStateCLK = digitalRead(_pin_CLK);
  
  if (currentStateCLK != lastStateCLK && currentStateCLK == LOW) {
    if (digitalRead(_pin_DT) == LOW) {
      // Вращение в плюс
      inputStatus.encoderDelta = 1;      
    } 
    else {
      // Вращение в минус
      inputStatus.encoderDelta = -1;      
    }
  }
  else {
    // Нет вращения
    inputStatus.encoderDelta = 0;
  }
}


void readButtons() {
  inputStatus.buttonPressed = 0;

  for (byte col = 0; col < numCols; col++) {
    digitalWrite(colPins[col], LOW); // активируем один столбец
    for (byte row = 0; row < numRows; row++) {
      if (digitalRead(rowPins[row]) == LOW) { // кнопка нажата        
        inputStatus.buttonPressed = keyMap[row][col];  // Передаем код кнопки в структуру хранения
      }
    }
    digitalWrite(colPins[col], HIGH); // отключаем столбец
  }
}



InputsStatus getInputStatus() {
  readEncoder();
  readButtons();

  return inputStatus;
}