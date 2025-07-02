// Библиотека опроса кнопок и энкодера микроволновой печи марки "BEKO". Заголовочный файл.

#include <stdint.h>
#ifndef INPUTS_H
#define INPUTS_H

#include <Arduino.h>

// Коды кнопок управления
#define BUTTON_SPEED_DEFROST 1
#define BUTTON_AUTO_DEFROST  2
#define BUTTON_MULTI_COOKING 3
#define BUTTON_WIGHT_ADJ     4
#define BUTTON_POWER         5
#define BUTTON_CLOCK         6
#define BUTTON_START         7
#define BUTTON_STOP          8

// Структура хранящая результат опроса кнопок и энкодера
struct InputsStatus {
  int encoderDelta;
  int buttonPressed;
};



void initEncoder(uint8_t pin_CLK, uint8_t pin_DT);
void initButtons(uint8_t colPin1, uint8_t colPin2, uint8_t rowPin1, uint8_t rowPin2, uint8_t rowPin3, uint8_t rowPin4);
InputsStatus getInputStatus();



#endif