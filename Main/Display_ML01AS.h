// Библиотека для дисплея "ML01AS-21" микроволновой печи марки "BEKO". Заголовочный файл.

#include <stdint.h>
#ifndef DISPLAY_ML01AS_H
#define DISPLAY_ML01AS_H

#include <Arduino.h>

#define CHAR_A 0   // A
#define CHAR_b 1   // b
#define CHAR_C 2   // C
#define CHAR_c 3   // c
#define CHAR_d 4   // d
#define CHAR_E 5   // E
#define CHAR_F 6   // F
#define CHAR_H 7   // H
#define CHAR_I 8   // I
#define CHAR_J 9   // J
#define CHAR_L 10  // L
#define CHAR_n 11  // n
#define CHAR_O 12  // O
#define CHAR_o 13  // o
#define CHAR_P 14  // P
#define CHAR_r 15  // r
#define CHAR_S 16  // S
#define CHAR_t 17  // t
#define CHAR_U 18  // U
#define CHAR_u 19  // u
#define CHAR_UNDERSCORE 20 // _
#define CHAR_DASH 21 // - 



void displayInit(uint8_t latchPin, uint8_t clockPin, uint8_t dataPin, uint8_t sym1Pin, uint8_t sym2Pin);
void displayClear();
void displayShowSymbols(uint16_t iconBits);
void displayShowNumber(int number);
void displayDigit(uint8_t digit, uint8_t pos);
void displayChar(uint8_t specChar, uint8_t pos);



#endif