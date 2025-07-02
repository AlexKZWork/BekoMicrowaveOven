// Библиотека для дисплея "ML01AS-21" микроволновой печи марки "BEKO"

#include "Display_ML01AS.h"

static uint8_t _latchPin;  // STCP на 74HC595
static uint8_t _clockPin;  // SHCP на 74HC595
static uint8_t _dataPin;   // DS на 74HC595
static uint8_t _sym1Pin;   // Два выхода с МК на катоды дисплея напрямую
static uint8_t _sym2Pin;

// Значения для сегментов чтобы зажечь символы цифр
const uint8_t digits[] = {
  0b11111010, // 0
  0b01100000, // 1
  0b11011100, // 2
  0b11110100, // 3
  0b01100110, // 4
  0b10110110, // 5
  0b10111110, // 6
  0b11100000, // 7
  0b11111110, // 8
  0b11110110  // 9
};

// Значения для сегментов чтобы зажечь символы алфавита и другие спец. символы
const uint8_t charset[] = {
  0b11101110, // A
  0b00111110, // b
  0b10011010, // C
  0b00011100, // c
  0b01111100, // d
  0b10011110, // E
  0b10001110, // F
  0b01101110, // H
  0b01100000, // I
  0b01110000, // J
  0b00011010, // L
  0b00101100, // n
  0b11111010, // O
  0b00111100, // o
  0b11001110, // p
  0b00001100, // r
  0b10110110, // S
  0b00011110, // t
  0b01111010, // U
  0b00111000, // u
  0b00010000, // _
  0b00000100  // -
};

// Структура описывающая остальную половину значков, которые частично управляются с МК а частично с 74HC595
struct Icon {
  uint16_t bitMask;
  uint8_t data;
  int pin;
};

#define ICONS_COUNT 9

Icon icons[ICONS_COUNT];



void initIconsStruct() {
  icons[0] = {0b0000001000000000, 0b01000000, _sym1Pin};
  icons[1] = {0b0000000100000000, 0b01000000, _sym2Pin};
  icons[2] = {0b0000000010000000, 0b00100000, _sym1Pin};
  icons[3] = {0b0000000001000000, 0b00100000, _sym2Pin};
  icons[4] = {0b0000000000100000, 0b00010000, _sym1Pin};
  icons[5] = {0b0000000000010000, 0b00010000, _sym2Pin};
  icons[6] = {0b0000000000001000, 0b00001000, _sym2Pin};
  icons[7] = {0b0000000000000100, 0b00000100, _sym1Pin};
  icons[8] = {0b0000000000000010, 0b00000100, _sym2Pin};
}



void displayInit(uint8_t latchPin, uint8_t clockPin, uint8_t dataPin, uint8_t sym1Pin, uint8_t sym2Pin) {
  _latchPin = latchPin;
  _clockPin = clockPin;
  _dataPin = dataPin;
  _sym1Pin = sym1Pin;
  _sym2Pin = sym2Pin;

  initIconsStruct();

  pinMode(_latchPin, OUTPUT);
  pinMode(_clockPin, OUTPUT);
  pinMode(_dataPin, OUTPUT);
  pinMode(_sym1Pin, OUTPUT);
  pinMode(_sym2Pin, OUTPUT);

  displayClear();
}



// Функция отправки данных на сдвиговые регистры подключенные последовательно
void sendData(uint8_t firstRegisterData, uint8_t secondRegisterData) {
  // Необходимо разделить сегменты наполовину и показывать по очереди (из-за большого потребляемого тока дисплеем микроволновки, 74HC595 не справляется)

  uint8_t firstHalf  = secondRegisterData & 0b00001111;  // Первая половина цифры
  uint8_t secondHalf = secondRegisterData & 0b11110000;  // Вторая половина цифры

  digitalWrite(_latchPin, LOW);  // Начало записи
  
  shiftOut(_dataPin, _clockPin, MSBFIRST, ~firstHalf);  // Данные на второй регистр обязательно инвертированные так как дисплей с общими анодами  
  shiftOut(_dataPin, _clockPin, LSBFIRST, firstRegisterData);  // Данные на первый регистр
  
  digitalWrite(_latchPin, HIGH);  // Конец записи
  delayMicroseconds(150);

  digitalWrite(_latchPin, LOW);
  
  shiftOut(_dataPin, _clockPin, MSBFIRST, ~secondHalf);
  shiftOut(_dataPin, _clockPin, LSBFIRST, firstRegisterData);
  
  digitalWrite(_latchPin, HIGH);
  delayMicroseconds(150);
}



void displayClear() {
  sendData(0, 0);
  digitalWrite(_sym1Pin, HIGH);
  digitalWrite(_sym2Pin, HIGH);
}



// Функция показывает числа на семисегментном дисплее исключая ноли слева
void displayShowNumber(int number) {
  int numberDigits[4];  // digits[0] — младший разряд, а digits[3] — старший
  int digitsCount = 0;

  // Получаем количество цифр в числе (без учета знака)
  if (number == 0) {
    digitsCount = 1;
  } else {
    int temp = abs(number);  // если возможны отрицательные числа
    while (temp > 0) {
      temp /= 10;
      digitsCount++;
    }
 }
  
  // Выделяем четыре цифры в числе number
  numberDigits[0] = number % 10;
  numberDigits[1] = (number / 10) % 10;
  numberDigits[2] = (number / 100) % 10;
  numberDigits[3] = (number / 1000) % 10;
  
  // Проходим циклом по количеству цифр в числе number
  int digitSelector = 1;  // Разряд для цифр чтобы передавать в регистр (на первом 74HC595, который управляет транзисторами для общих анодов, первый и два последних вывода не подключены)
  for (int i = 0; i < digitsCount; i++) {
    digitSelector++;
    if (digitSelector == 4) {  // 4 разряд на дисплее используется для символов а не для сегментов (особенности разводки дисплея), пропускаем
      digitSelector++;
    }    
    sendData(1 << digitSelector, digits[numberDigits[i]]);  // Отправляем данные в регистры, сдвиг чтобы выбирать только нужные транзисторы
  }
}



// Функция включения одного значка на дисплее (все значки кроме правого столбца из за особенности разводки платы)
void displayIcon(uint8_t firstRegisterData, int controllerPin) {
  sendData(firstRegisterData, 0b00000000);
  digitalWrite(controllerPin, LOW);
  delayMicroseconds(150);
  digitalWrite(controllerPin, HIGH);  
}



/*
  Функция принимает биты в формате uint16_t для включения значков на дисплее. 1 = включен, 0 = выключен
  Биты для включения символов на дисплее (биты перечисляются слева направо) :
  1 - символ замок
  2 - двоеточие верхняя точка (для часов)
  3 - двоеточие нижняя точка (для часов)
  4 - кастрюля с паром (на дисплее справа)
  5 - символ 'g'
  6 - символ '%'
  7 - символ 'Auto'
  8 - символ разморозка
  9 - символ на дисплее сверху второй (непонятно, что изображено)
  10 - символ на дисплее снизу второй (волны)
  11 - символ на дисплее сверху третий в виде <|>
  12 - символ снизу 1s
  13 - символ кастрюля с паром (внизу)
  14 - символ часы
  15 - символ снизу 2s
  16 - не используется (необходимо выставить в 0)

  Для примера - 0b1000010000000000 включит значок замка и значок процента
*/
  
void displayShowSymbols(uint16_t iconBits) {
  // Первые 6 бит обрабатываются через сдвиговые регистры

  uint8_t firstIcons = ((iconBits >> 10) << 2);  // Вырезаем первые 6 бит и сдвигаем влево на 2 бита чтобы заполнить двумя нолями справа (необходимо для 74HC595) 
  if ((firstIcons & 0b00000100) != 0) {  // Шестой бит в дисплее не используется, нужно его перенести в пятый (из за схемотехники)
    firstIcons |= (1 << 1);  // Записываем седьмой бит
  }  
  sendData(0b00010000, firstIcons);  // Необходимый анод дисплея включается четвертым битом

  // Отслеживаем оставшиеся биты значков
  for (int i = 0; i < ICONS_COUNT; i++) {
    if (iconBits & icons[i].bitMask) {
      displayIcon(icons[i].data, icons[i].pin);
    }
  }
}



// Функция показывает цифру в определенном разряде (от 1 до 4)
void displayDigit(uint8_t digit, uint8_t pos) {
  uint8_t shiftSelector;
  // Нужно развернуть для вычисления позиции. Позиции считаются слева от 1 до 4
  if (pos > 2) {
    shiftSelector = 6 - pos;  // для разрядов, где особая разводка
  } else {
    shiftSelector = 7 - pos;
  }
  uint8_t digitSelector = (1 << shiftSelector);  // Включаем бит обозначающий позицию
  
  sendData(digitSelector, digits[digit]);
}



// Функция показывает символы алфавита и др. в определенном разряде (от 1 до 4)
void displayChar(uint8_t specChar, uint8_t pos) {
  uint8_t shiftSelector;
  // Нужно развернуть для вычисления позиции. Позиции считаются слева от 1 до 4
  if (pos > 2) {
    shiftSelector = 6 - pos;  // для разрядов, где особая разводка
  } else {
    shiftSelector = 7 - pos;
  }
  uint8_t digitSelector = (1 << shiftSelector);  // Включаем бит обозначающий позицию
  
  sendData(digitSelector, charset[specChar]);
}