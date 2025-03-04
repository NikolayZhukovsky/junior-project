#include <Wire.h>                        // Подключаем библиотеку для работы с устройствами по I2C
#include <microDS3231.h>                 // Подключаем библиотеку для работы с модулем часов реального времени
#include <LiquidCrystal_I2C.h>           // Подключаем библиотеку для работы с ЖК-дисплеем 2004А
#include <GyverEncoder.h>                // Подключаем библиотеку для работы с энкодером KY-040
#include <GyverButton.h>                 // Подключаем библиотеку для работы с тактовой кнопкой
#include <microDS18B20.h>                // Подключаем библиотеку для работы с датчиком температуры DS18B20
#include <EEPROM.h>                      // Подключаем библиотеку для работы с EEPROM-памятью

#define POT     A0                       // Определяем номер аналогового входа,  к которому подключен  фоторезистор                   (сейчас А0)
#define SENSOR  A1                       // Определяем номер аналогового входа,  к которому подключен  датчик влажности почвы         (сейчас А1)
#define CLK      3                       // Определяем номер цифрового   входа,  к которому подключен  контакт CLK энкодера         (сейчас 3pin)
#define DT       4                       // Определяем номер цифрового   входа,  к которому подключен  контакт DT энкодера          (сейчас 4pin)
#define SW       5                       // Определяем номер цифрового   входа,  к которому подключен  контакт SW энкодера          (сейчас 5pin)
#define MOTOR    7                       // Определяем номер цифрового   выхода, к которому подключена водяная помпа                (сейчас 8pin)
#define LIGHT    8                       // Определяем номер цифрового   выхода, к которому подключена подсветка                    (сейчас 7pin)
#define VCC_POT  9                       // Определяем номер цифрового   выхода, к которому подключено питание дачтика влажности    (сейчас 9pin)
#define BTN_PIN 10                       // Определяем номер цифрового   входа,  к которому подключена кнопка (вкл/выкл)           (сейчас 10pin)
#define LED     11                       // Определяем номер цифрового   выхода, к которому подключен  светодиод-индикатор         (сейчас 11pin)
#define VCC_LEV 12                       // Определяем номер цифрового   выхода, к которому подключено питание датчика уровня воды (сейчас 12pin)
#define LEVEL    6                       // Определяем номер цифрового   входа,  к которому подключен  датчик уровня воды          (сейчас 13pin)


#define SSID_ADDR  50                    // Адрес хранения имени  сети Wi-Fi в EEPROM
#define PASS_ADDR 200                    // Адрес хранения пароля сети Wi-Fi в EEPROM

#define HUM_INDEX_ADDR   400             // Адрес хранения индекса массива поддерживаемой влажности        в EEPROM (байт - 0/1/2) для NodeMCU
#define STAGE_INDEX_ADDR 401             // Адрес хранения индекса массива периода времени между порливами в EEPROM (байт - 0...8) для NodeMCU
#define DURE_INDEX_ADDR  402             // Адрес хранения индекса массива продолжительности полива        в EEPROM (байт - 0...7) для NodeMCU
#define HUM_ADDR         300             // Адрес хранения поддерживаемой влажности                        в EEPROM (байт - 0/1/2)
#define STAGE_ADDR       308             // Адрес хранения периода времени между порливами                 в EEPROM (в часах)
#define DURE_ADDR        313             // Адрес хранения продолжительности полива                        в EEPROM (в секундах)

#define ST_TIME_ADDR     320             // Адрес хранения времени включения  подсветки в EEPROM (в минутах от начала дня)
#define END_TIME_ADDR    325             // Адрес хранения времени выключения подсветки в EEPROM (в минутах от начала дня)
#define ILLUM_ADDR       330             // Адрес хранения поддерживаемой  освещенности в EEPROM (в процентах - от 0 до 99)

#define WATER_ADDR       340             // Адрес хранения текущего состояния автополива    в EEPROM (1 - вкл; 0 - выкл)
#define LIGHT_ADDR       341             // Адрес хранения текущего состояния автоподсветки в EEPROM (1 - вкл; 0 - выкл)

#define  MIN  450                        // Определяем минимальное  показание датчика влажности почвы (в воздухе)
#define  MAX  180                        // Определяем максимальное показание датчика влажности почвы    (в воде)

GButton but(BTN_PIN);                    // Объявляем объект but для работы с кнопкой включения/выключения экрана
MicroDS3231 rtc;                         // Объявляем объект rtc для работы с модулем на базе чипа DS3231, используется аппаратная шина I2C
LiquidCrystal_I2C lcd(0x27,20,04);       // Задаем адрес (в формате 0х**) и размерность дисплея
Encoder enc(CLK, DT, SW, TYPE2);         // Объявляем объект enc для работы с модулем энкодера KY-040 и сразу выбираем тип ("однотактовый")
MicroDS18B20<2> sensor;                  // Обяляем обьеект для работы с датчиком тмпературы DS18B20

uint16_t val_sensor;                     // Создаем переменную для хранения адаптированных показаний, считанных с датчика влажности почвы
uint16_t val_pot;                        // Создаем переменную для хранения адаптированных показаний, считанных с фоторезистора
uint16_t val_temp;                       // Создаем переменную для хранения адаптированных показаний, считанных с датчика температуры
DateTime currTime;                       // Создаём обьект хранящий текущее время на устройстве

unsigned long timing_survey = 0;         // Создаем вспомогательную переменную для хранения времени (для считывания значений с датчиков)
unsigned long timing_render = 0;         // Создаем вспомогательную переменную для хранения времени (для отрисовки экрана)
unsigned long timing_time   = 0;         // Создаем вспомогательную переменную для хранения времени (для считывания времени с модуля часов)
unsigned long timing_light  = 0;         // Создаем вспомогательную переменную для хранения времени (для проверки освещенности в автоподсветке) 
unsigned long motor_time    = 0;         // Создаем вспомогательную переменную для хранения времени (для считвания влажности в автополиве)
unsigned long time_off      = 0;         // Создаем вспомогательную переменную для хранения времени (для автовыключения экрана)
int time_lcd_is_on      = 90000;         // Переменная, хранящая продолжительность работы экрана до его выключения

boolean motorState = false;              // Создаём переменную, хранящую текущее состояние водяной помпы (true - влючена; false - выключена)
boolean lightState = false;              // Создаём переменную, хранящую текущее состояние подсветки     (true - влючена; false - выключена)
byte val;                                // Создаём переменную, хранящую код, полученный с NodeMCU (1 байт информации - символ)                     

const String days[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};                             // Массив с сокращениями дней недели
const byte customChar[] = { B00000, B00100, B01110, B11111, B11111, B01110, B00100, B00000 }; // Символ-указатель

String ssID = "";                        // Имя сти Wi-Fi
String PASS = "";                        // Пароль сети Wi-Fi 
bool flag;                               // Вспомогательная переменная (нужна для реализации выхода из установки параметров новой сети Wi-Fi)
byte len, pos;                           // Вспомогательные переменные для раздела установки нового WiFi

uint8_t LEN = 8;                         // Создаём переменную, хранящую длину массива DATA_SCREEN
String DATA_SCREEN[8] = {"                    ", 
                         "Temperature: xx.xx*C", 
                         "Luminosity:  xx%", 
                         "Soil Hum.:   xx%",
                         "",
                         "Autowater    ON ",
                         "Autolight    ON ",
                         "Settings"};

uint8_t LEN_2 = 7;                       // Создаём переменную, хранящую длину массива DATA_SETT
const String DATA_SETT[7] = {"SETTINGS (press the",                         
                         "button to select)",
                         "",
                         "Curr  WiFi",
                         "New   WiFi",
                         "CODE: 3242",
                         String(char(127)) + "Exit"};                   

int8_t POS = 0;                          // Создаем переменную для хранения текущей позиции в основном меню
int8_t POS_2 = 0;                        // Создаем переменную для хранения текущей позиции в разделе настройки
int8_t POS_3 = 0;                        // Создаем переменную для хранения текущей позиции в разделе установки новой сети

int  supp_hum;                           // Хранится поддерживаемая влажность в автополиве (0/1/2)
int  stage;                              // Хранится период между поливами    в автополиве (в часах)
int  duration;                           // Хранится продолжительность полива в автополиве (в секундах)

byte dure_index, stage_index, hum_index; // Переменные для хранения индексов массивов для NodeMCU

int st_time, end_time;                   // Время включения и выключения автоподсветки (в минутах относительно начала дня)
byte illum;                              // Поддерживаемая освещенность в процентах (от 0 до 99) для автоподсветки

int on_time = 816;                       // Время первого включения автополива
byte on_day;                             // День  первого включения автополива
bool autowater_state, autolight_state;   // Состояния автополива и автоподсветки соответственно (0 - выкл; 1- вкл)
bool screenState = true;                 // Состояние экрана (0 - выкл; 1 - вкл)
bool level;                              // 
bool isDataScreen = true;                // Переменная, хранящая положение в интерфейсе (является ли текущий раздел базовым)

const char str[67] PROGMEM = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-[]"; // Массив всех символов, используемых в именовании имени и пароля сети



void setup() {
  lcd.init();                            // Инициализация lcd 
  lcd.createChar(0, customChar);         // Создаём свой специальный символ для "курсора" (строчкой выше его код)

  pinMode(LED,     OUTPUT);              // Усатнавливаем "выход" на контакте, связанного с светодиодом-индикатором
  pinMode(VCC_LEV, OUTPUT);              // Усатнавливаем "вход"  на контакте, связанного с питанием датчика уровня воды
  pinMode(LEVEL,    INPUT);              // Усатнавливаем "вход"  на контакте, связанного с датчиком уровня воды
  pinMode(POT,      INPUT);              // Усатнавливаем "вход"  на контакте, связанного с фоторезистором
  pinMode(VCC_POT, OUTPUT);              // Усатнавливаем "выход" на контакте, связанного с питанием датчика влажности почвы
  pinMode(LIGHT,   OUTPUT);              // Усатнавливаем "выход" на контакте, связанного с подсветкой
  pinMode(MOTOR,   OUTPUT);              // Усатнавливаем "выход" на контакте, связанном  с водяной помпой
  digitalWrite(LED,     LOW);            // Устанавливаем низкий  логический уровень
  digitalWrite(VCC_POT, LOW);            // Устанавливаем низкий  логический уровень на выходе питания датчика влажности почвы
  digitalWrite(LIGHT,   LOW);            // Устанавливаем низкий  логический уровень на выходе подсветки     (при подаче минуса начаинает работать)
  digitalWrite(MOTOR,   LOW);            // Устанавливаем низкий  логический уровень на выходе водяной помпы (при подаче минуса начаинает работать)
  
  Serial.begin(115200);                  // Открываем последовательный порт на скорости 115200 бод

  EEPROM.get(HUM_ADDR,  supp_hum);       // Получаем поддерживаемую влажность  из EEPROMа
  EEPROM.get(STAGE_ADDR,   stage);       // Получаем период между поливами     из EEPROMа
  EEPROM.get(DURE_ADDR, duration);       // Получаем продолжительность поливов из EEPROMа

  EEPROM.get(ST_TIME_ADDR,   st_time);   // Получаем время включения  из EEPROMа
  EEPROM.get(END_TIME_ADDR, end_time);   // Получаем время выключения из EEPROMа
  EEPROM.get(ILLUM_ADDR,       illum);   // Получаем освещенность     из EEPROMа
  
  byte water_bool;                       // Создём переменную для состояния автополива (в численной интепритации 0/1)
  EEPROM.get(WATER_ADDR, water_bool);    // Кладём в неё текущее значение 
  
  if (water_bool) { autowater_state = true;  }                                      // Интерпитируем в логический тип...
  else            { autowater_state = false; DATA_SCREEN[5] = "Autowater    OFF"; } // ...
  
  byte light_bool;                                                                  // Создём переменную для состояния автоподсветки (в численной интепритации 0/1)
  EEPROM.get(LIGHT_ADDR, light_bool);                                               // Кладём в неё текущее значение   
  
  if (light_bool) { autolight_state = true;  }                                      // Интерпитируем в логический тип...
  else            { autolight_state = false; DATA_SCREEN[6] = "Autolight    OFF"; } // ...
   
  reading_time();                                                                   // Считываем текущее время (БУДЕТ ЛИ РАБОТАТЬ БЕЗ ЭТОГО???)
  reading_soil_hum();                                                               // Считываем текущюю влажность почвы

  on_time = currTime.hour * 60 + currTime.minute + 30;
  if (on_time > 1440) on_time -= 1440;

  if (currTime.hour * 60 + currTime.minute < on_time) on_day = currTime.day;        // Время включения автополива - сегодняшний день
  else { on_day = currTime.day + 1; if (on_day == 8) on_day = 1; }                  // Время включения автополива - завтрашний день

  Serial.println();                                                                                                   // Перенос строки
  Serial.println("WATER " + String(supp_hum) + " " + String(stage)    + " " + String(duration));                      // Выводим всё об автополиве
  Serial.println("LIGHT " + String(st_time)  + " " + String(end_time) + " " + String(illum));                         // Выводим всё об автоподсетке
  Serial.println("New on is " + days[on_day - 1] + " " + String(on_time / 60) + ":" + String (on_time % 60));         // Новое включение полива
  Serial.println("Now is " + days[currTime.day - 1] + " " + String(currTime.hour) + ":" + String (currTime.minute));  // Текущее время
  Serial.println();                                                                                                   // Перенос строки
  
  water_level();                                                                         // Проверка уровня воды

  if (screenState) { lcd.backlight(); hello_screen(); }                                  // Если состояние экрана true, печатаем приветствие
  else             { lcd.clear();  lcd.noBacklight(); }                                  // Иначе его очищаем и отключаем подсветку
  delay(350);                                                                            // Пауза 350 мс (+700мс в функции hello_screen())
}



void loop() {
  enc.tick();                                                                            // Обязательная функция отработки кнопки вкл/выкл экрана (должна постоянно опрашиваться)

  if (not isDataScreen)  { isDataScreen = true; }                                        // Инвертируем значение
  
  if (millis() - time_off > time_lcd_is_on) {                                            // Если экран горит больше установленного времени...
    screenState = false; lcd.clear(); POS = 0; lcd.noBacklight();                        // ...то выключаем его
  }
  
  if (not screenState and enc.isPress()) { btn_for_lcd(); }                                                  // Если кнопка нажата, то обрабатываем данноне событие
  
  if (screenState) enc.tick();                                                           // Обязательная функция отработки энкодера (должна постоянно опрашиваться)
  handleData();                                                                          // Функция считывания данных с NodeMCU     (должна постоянно опрашиваться)
  if (enc.isTurn()) {                                                                    // Если была повертнута ручка энкодера...
    time_off = millis(); POS = rendering_after_rotation(DATA_SCREEN, POS, LEN);          // ...обрабатываем поворот ручки энкодера     (меняются данные на LCD) 
  }

  if (POS + 3 == 3 and enc.isPress()) {                                                  // Если нажата кнопка на энкодере напротив текущей влажности почвы...
    reading_soil_hum(); reading_sensor_values();                                         // Считываем текущие значения датчиков
    rendering(DATA_SCREEN, POS);                                                         // Отбражаем новые данные в LCD
    time_off = millis();
  }
  
  if (POS + 3 == 4 and enc.isPress()) { time_off = millis(); }                           // Если нажата кнопка напротив пробела, то ничего не происходит
  
  if (POS + 3 == 5 and enc.isPress()) {                                                  // Если нажата кнопка на энкодере в строке с включением/выклюением водяной помпы
    autowater_state = !autowater_state;                                                  // При нажатии кнопки инвертируем состояние автополива
    if (autowater_state) { DATA_SCREEN[5] = "Autowater    ON";                }          // Если состояние водяной помпы true, то меняем масив для отрисовки в LCD
    else                 { DATA_SCREEN[5] = "Autowater    OFF"; motor_off();  }          // Если состояние водяной помпы false, то меняем масив для отрисовки в LCD и выключаем водяную помпу
    if (autowater_state) EEPROM.put(WATER_ADDR, 1); else EEPROM.put(WATER_ADDR, 0);      // Кладём новое состояние автополива в EEPROM
    rendering(DATA_SCREEN, POS); time_off = millis();                                    // Отрисовваем изменившийся экран LCD
  }
  
  if (POS + 3 == 6 and enc.isPress()) {                                                  // Если нажата кнопка на энкодере в строке с включением/выклюением подсветки
    autolight_state = !autolight_state;                                                  // При нажатии кнопки инвертируем состояние подсветки на противопложное
    if (autolight_state) { DATA_SCREEN[6] = "Autolight    ON";  timing_light = 0; }      // Если состояние водяной помпы true, то меняем масив для отрисовки в LCD
    else                 { DATA_SCREEN[6] = "Autolight    OFF"; light_off();  }          // Если состояние водяной помпы false, то меняем масив для отрисовки в LCD и выключаем подсветку
    if (autolight_state) EEPROM.put(LIGHT_ADDR, 1); else EEPROM.put(LIGHT_ADDR, 0);      // Кладём новое состояние автоподсветки в EEPROM
    rendering(DATA_SCREEN, POS); time_off = millis();                                    // Отрисовваем изменившийся экран LCD
  }

  
  if (POS + 3 == 7 and enc.isPress()) {                                                  // Если нажата кнопка на энкодере в строке с надписью настройки
    POS_2 = 0; time_off = millis(); isDataScreen = false;                                // Начальная позиция перемещения по настройкам равна 0
    rendering(DATA_SETT, POS_2);                                                         // Отрисовываем настройки в LCD
    while (true) {                                                                       // Выполнение может быть бесконечным
      but.tick();                                                                        // Обязательная функция отработки кнопки вкл/выкл экрана (должна постоянно опрашиваться)
      if (but.isPress()) { btn_for_lcd(); break; }                                       // Если нажата кнопка, обрабатываем событие
      if (millis() - time_off > time_lcd_is_on) {                                        // Если экран горит больше установленного времени...
        screenState = false; lcd.clear(); POS = 0; lcd.noBacklight(); break;             // ...то выключаем его
      }
      if (screenState) enc.tick();                                                       // Обязательная функция отработки энкодера (должна постоянно опрашиваться)
      handleData();                                                                      // Функция считывания данных с NodeMCU     (должна постоянно опрашиваться)
      if (enc.isTurn()) { 
        time_off = millis(); POS_2 = rendering_after_rotation(DATA_SETT, POS_2, LEN_2);  // Отрисовываем экран после поворта ручки энкодера
      }
      if (POS_2 == 3 and enc.isPress()) {                                                // Если нажата кнопка энкодера напротив надписи exit...
        time_off = millis(); rendering(DATA_SCREEN, POS); break;                         // ...то отрисовываем основное меню и выходим из текущего 
      }          
      if (POS_2 == 1 and enc.isPress()) { time_off = millis(); set_wifi(); }             // Если нажата кнопка энкодера напротив надписи New FiFi, то обрабатываем событие 
      if (POS_2 == 2 and enc.isPress()) { time_off = millis(); 
        ee_write(SSID_ADDR, "PerpetuumMobile"); ee_write(PASS_ADDR, "Plo14Ar04Tem92");
        Serial.write(20);                                                                // Печатаем упраляющий символ - установку новой сети на NodeMCU
        Serial.println(ssID); Serial.println(PASS);
      }                         
      if (POS_2 == 0 and enc.isPress()) {                                                // Если кнопка нажата напротив надписи Curr WiFi...
        time_off = millis();
        lcd.clear();                                                                     // Очищаем LCD
        String ssid = ee_read(SSID_ADDR); String pass = ee_read(PASS_ADDR);              // Читаем имя и пароль сети из EEPROMа
        Serial.println(ssid); Serial.println(pass);                                      // Выводим их в серийный порт
        lcd.setCursor(0, 0); lcd.print("SSID"); lcd.setCursor(0, 2); lcd.print("PASS");  // На 1ой строчке LCD печатем SSID, а на 3ей PASS
        if (ssid == "") { lcd.setCursor(0, 1); lcd.print("Not chosen"); }                // Если имя сети не было вбито, извещаем об этом
        if (ssid)       { lcd.setCursor(0, 1); lcd.print(ssid);         }                // Иначе печатаем его
        if (pass == "") { lcd.setCursor(0, 3); lcd.print("Not chosen"); }                // Если пароль сети не был вбит, извещаем об этом
        if (pass)       { lcd.setCursor(0, 3); lcd.print(pass);         }                // Иначе печатаем его
        while (true)    {                                                                // Оставляем данные на сколь угодное большое время
          but.tick(); if (but.isPress()) { btn_for_lcd(); break; }                       // Если кнопка наждата, то обрабатываем данноне событие
          if (millis() - time_off > time_lcd_is_on) {                                    // Если экран горит больше установленного времени...
            screenState = false; lcd.clear(); POS = 0; lcd.noBacklight(); break;         // ...то выключаем его
          }
          enc.tick(); if (enc.isPress()) {                                               // Если кнопка энкодера нажата...
            time_off = millis(); rendering(DATA_SETT, POS_2); break;                     // ...то возвращаемся в меню настроек
          }        
        }  
      }
    }
  }  

  if (millis() <= 1) {                                                                   // Если произошло переполнение фукнции millis()...
    timing_survey = 0; timing_render = 0;                                                // То обнуляем четыре переменных времени...
    timing_time = 0; timing_light = 0; time_off = 0;                                     // ...
  }
    
  if (screenState and millis() - timing_survey > 300000 or timing_survey == 0) {         // Меняя значение, меняем период опроса датчиков
    timing_survey = millis();   reading_sensor_values();                                 // Вызываем ункцию считывания показаний с датчиков
  }

  if (millis() - timing_time > 60000 or timing_time == 0) {                              // Меняя значение, меняем период считывания с часов реального времени
    timing_time = millis();     reading_time();                                          // Вызываем функцию считывания текущего времени

    if (currTime.hour * 60 + currTime.minute == 780) {
      water_level(); 
      reading_soil_hum(); 
      if (val_sensor < supp_hum) { 
        Serial.println("LOW HUM"); 
        Serial.write(64); Serial.print(String(val_sensor));
      }
    }
    
    if (autolight_state) {                                                               // Если автоподсветка включена...
      if (currTime.hour * 60 + currTime.minute == st_time) {                             // И если текущее время - время включения подсветки...
        light_on(); timing_light == 0;                                                   // То включаем её и обнуляем переменную для опроса датчика освещенности
      }
      
      if (currTime.hour * 60 + currTime.minute == end_time) { light_off(); }             //  Если время выключения подсвтеки, то выключаем её
    }
    
  
    if (currTime.hour * 60 + currTime.minute == on_time and currTime.day == on_day) {    // Если текущее время и день совпадает со временем включения полива
      on_time += stage * 60;                                                             // Ко времени включения прибавлаяем период между поливами (в часах), умноженный на 60
      on_day += on_time / 1440; if (on_day > 7) on_day -= 7;                             // Если вышли за пределы текущего дня переводим день, если за неделю, то неделю
      on_time %= 1440;                                                                   // Получаем время включения в минутах остатком на длину дня в минутах
      Serial.println("New on is " + days[on_day - 1] + " " + String(on_time / 60) + ":" + String (on_time % 60)); // Печатаем новое время и день включения полива
      if (autowater_state) {                                                             // Если автополив включён...
        motor_on(); motor_time = millis();                                               // Включаем его
        Serial.println("MOTOR is ON");                                                   // Печатаем сообщение о включении полива
      }
    }
  }


  if (screenState and millis() - timing_render > 15000 or timing_render == 0) {          // Меняя значение, меняем период отрисовки экрана
    timing_render = millis(); rendering(DATA_SCREEN, POS);                               // Вызов фунции отрисовки экрана
  }

  if (autolight_state) {                                                                 // Если автоподсветка включена...
    if (must_on() and millis() - timing_light > 7000) {                                  // И если подсвтека должна быть включена и прошло n минут...
      Serial.println("Light state " + String(lightState));                               // Печатаем текущее состояние подсветки
      
      timing_light = millis(); reading_sensor_values();                                  // Читаем значение освещённости
      if (lightState and val_pot > illum)     { light_off(); }                           // Если подсвтека включена и текущая яркость больше установленной, выключаем её
      if (not lightState and val_pot < illum) { light_on();  }                           // Если была выключена и текущая яркость меньше установленной, включаем её
    }
  }
  
  if (autowater_state) {                                                                 // Если автополив включён...
    if (motorState) { Serial.println((millis() - motor_time) / 1000); Serial.print(duration); }
    if (motorState and (millis() - motor_time) / 1000 > duration) {                      // И если истекло время продолжительности полива...
      motor_off(); Serial.println("MOTOR is OFF all Time");                              // Выключаем его и печатем сообщение в серийный порт
    }
  } 
}



void btn_for_lcd() {                                                                   
  screenState = !screenState;                                                            // Инвертируем текущее состояние экрана
  if (screenState) {                                                                     // Если экран был выключен...
    lcd.backlight(); hello_screen(); rendering(DATA_SCREEN, POS);                        // Печатаем приветствие, включаем экран
    time_off = millis(); water_level();                                                  // Проверяем уровень воды
  }  
  else             { lcd.clear(); POS = 0; lcd.noBacklight(); }                          // Иначе очищаем экран и выключаем его подсветку
}

void hello_screen() {                                                                              // Функция вывода приветствия в LCD
  lcd.backlight();                                                                                 // Включаем подсветку
  lcd.clear();                                                                                     // Очищаем вывод ЖК
  lcd.setCursor(3, 1);                                                                             // Устанавливаем курсор в 3 позицию 2 строки
  lcd.print("Weather station");                                                                    // Выводим текст
  lcd.setCursor(5, 3);                                                                             // Устанавливаем курсор в 6 позицию 4 строки
  lcd.print("July   2024");                                                                        // Выводим текст
  delay(700);                                                                                      // Задержка 700 мс
} 

bool must_on() {                                                                                   // Функция проверки того, что подсветка должна была бы быть сейчас включена
  return st_time < currTime.hour * 60 + currTime.minute and currTime.hour * 60 + currTime.minute < end_time; // Возвращает булево значение
}

void light_on()  { lightState = true;  digitalWrite(LIGHT, HIGH);  }                                // Функция включения  подсветки
void light_off() { lightState = false; digitalWrite(LIGHT, LOW); }                                // Функция выключения подсветки
void motor_on()  { motorState = true;  digitalWrite(MOTOR, HIGH);  }                                // Функция включения  автополива 
void motor_off() { motorState = false; digitalWrite(MOTOR, LOW); }                                // Функция выключения полива


void handleData() {                                                                                // Функция считывания данных с NodeMCU     (должна постоянно опрашиваться)
  if (Serial.available() > 0) {                                                                    // Если серийный порт не пустой...
    val = Serial.read();                                                                           // Чиатем управляющий байт информации
    
    if (val == 25) {                                                                               // Команда включения автоподсветки
      DATA_SCREEN[6] = "Autolight    ON";  autolight_state = true;  timing_light = 0;              // Включаем автоподсветку
      if (autolight_state) EEPROM.put(LIGHT_ADDR, 1); else EEPROM.put(LIGHT_ADDR, 0);              // Пишем текущее состояние автоподсветки в EEPROM
    } 
    if (val == 63)  {                                                                               // Команда выключения автоподсветки
      DATA_SCREEN[6] = "Autolight    OFF"; autolight_state = false; light_off();                   // Выключаем автоподсветку
      if (autolight_state) EEPROM.put(LIGHT_ADDR, 1); else EEPROM.put(LIGHT_ADDR, 0);              // Пишем текущее состояние автоподсветки в EEPROM
    }                               
    if (val == 4)  {                                                                               // Команда включения автополива
      DATA_SCREEN[5] = "Autowater    ON";  autowater_state = true;                                 // Включаем автополив
      if (autowater_state) EEPROM.put(WATER_ADDR, 1); else EEPROM.put(WATER_ADDR, 0);              // Пишем текущее состояние автополива в EEPROM
    }         
    if (val == 5)  {                                                                               // Команда выключения автополива
      DATA_SCREEN[5] = "Autowater    OFF"; autowater_state = false; motor_off();                   // Выключаем автополив
      if (autowater_state) EEPROM.put(WATER_ADDR, 1); else EEPROM.put(WATER_ADDR, 0);              // Пишем текущее состояние автополива в EEPROM
    }
    if (val == 25 or val == 3 or val == 4 or val == 5) {                                           // Если команды - вкл/выкл...
      if (screenState and isDataScreen) { rendering(DATA_SCREEN, POS); }                           // ...то отрисовываем экран
    }

    if (val == 7) {                                                                                // Команда запроса освещенности и температуры
      reading_soil_hum(); reading_sensor_values(); Serial.println(String(val_pot) + "," + String(val_temp) + "," + String(val_sensor));  // Читаем данные и пишем в серийный порт
    }
    
    if (val == 8) {                                                                                // Установка нового времени
      delay(50); DateTime t;                                                                       // Ждём 50 мс, создаём обьект времени
      t.hour = Serial.parseInt(); t.minute = Serial.parseInt(); t.second = Serial.parseInt();      // Парсим данные...
      t.date  = Serial.parseInt(); t.month =  Serial.parseInt(); t.year =   Serial.parseInt();     // ...
      t.day = Serial.parseInt();                                                                   // ...
      rtc.setTime(t);                                                                              // Устанавливаем новое время и считываем его
      reading_time();
    }
    
    if (val == 9) {                                                                                // Команда запроса текущего времени
      reading_time();                                                                              // Читаем текущее время
      String res = String(currTime.date / 10) + String(currTime.date % 10) + ".";                   // Формируем ответ - время, дату и день недели...
      res += String(currTime.month / 10) + String(currTime.month % 10) + ".";                      // ...
      res += String(currTime.year) + "    ";                                                       // ... 
      res += String(currTime.hour / 10) + String(currTime.hour % 10) + ":";                        // ... 
      res += String(byte(currTime.minute) / 10) + String(byte(currTime.minute) % 10) + ":";        // ...
      res += String(currTime.second / 10) + String(currTime.second % 10) + " ";                    // ...
      res += days[currTime.day - 1];                                                              // ...
      Serial.print(res); res = "";                                                                 // Печатаем результат в серийный порт
    }

    if (val == 36) {                                                                               // Команда обновления данных автополива
      delay(100);                                                                                  // Задержка перед приёмом данных
      supp_hum = Serial.parseInt(); if (supp_hum) EEPROM.put(HUM_ADDR,   supp_hum - 1);            // Парсим данные и записываем в EEPROM...
      stage    = Serial.parseInt(); if (stage)    EEPROM.put(STAGE_ADDR, stage);                   // ...
      duration = Serial.parseInt(); if (duration) EEPROM.put(DURE_ADDR,  duration);                // ...
      
      on_time = currTime.hour * 60 + currTime.minute + 30;
      if (on_time > 1440) on_time -= 1440;
  
      if (currTime.hour * 60 + currTime.minute < on_time) on_day = currTime.day;                   // Время включения автополива - сегодняшний день
      else { on_day = currTime.day + 1; if (on_day == 8) on_day = 1; }                             // Время включения автополива - завтрашний день
      Serial.println("\nWATER is set /" + String(supp_hum) + "/ /" + String(stage) + "/ /" + String(duration) + "/");                                                            // Печатаем сообщение об обновлении данных в серийный порт
      Serial.println("New on is " + days[on_day - 1] + " " + String(on_time / 60) + ":" + String (on_time % 60));
      Serial.println("Now is " + days[currTime.day - 1] + " " + String(currTime.hour) + ":" + String (currTime.minute));
    }

    if (val == 64) {                                                                               // Команда обновления данных индексов массивов для NodeMCU 
      delay(100);                                                                                  // Задержка перед приёмом данных
      hum_index   = Serial.parseInt(); if (hum_index)   EEPROM.put(HUM_INDEX_ADDR,   hum_index);   // Парсим данные и записываем в EEPROM...
      stage_index = Serial.parseInt(); if (stage_index) EEPROM.put(STAGE_INDEX_ADDR, stage_index); // ...
      dure_index  = Serial.parseInt(); if (dure_index)  EEPROM.put(DURE_INDEX_ADDR,  dure_index);  // ...
      Serial.println("\nINDEX is set");                                                            // Печатаем сообщение об обновлении данных в серийный порт
    }

    if (val == 17) {                                                                               // Команда запроса данных индексов массивов для NodeMCU 
      EEPROM.get(HUM_ADDR,   supp_hum);                                                            // Парсим данные из EEPROM...
      EEPROM.get(STAGE_INDEX_ADDR, stage_index);                                                   // ...
      EEPROM.get(DURE_INDEX_ADDR,  dure_index);                                                    // ...
      Serial.print(String(supp_hum) + "," + String(stage_index) + "," + String(dure_index));       // Печатаем в серийный порт
    }

    if (val == 60) {                                                                               // Команда обновления данных автоподсветки
      delay(50);                                                                                   // Задержка перед приёмом данных
      st_time  = Serial.parseInt(); EEPROM.put(ST_TIME_ADDR,  st_time);                            // Парсим данные и записываем в EEPROM...
      end_time = Serial.parseInt(); EEPROM.put(END_TIME_ADDR, end_time);                           // ...
      illum    = Serial.parseInt(); EEPROM.put(ILLUM_ADDR,    illum);                              // ... 
      if (autolight_state and must_on()) {
        timing_light = millis(); reading_sensor_values();                                          // Читаем значение освещённости
        if (lightState and val_pot > illum)     { light_off(); }                                   // Если подсвтека включена и текущая яркость больше установленной, выключаем её
        if (not lightState and val_pot < illum) { light_on();  }                                   // Если была выключена и текущая яркость меньше установленной, включаем её
      }
      if (not must_on()) { light_off(); }                                                          // Если подсветка должна быть выключена, то выключаем её
      
      Serial.println("\nTIME is set");                                                             // Печатаем сообщение об обновлении данных в серийный порт
    }

    if (val == 19) {                                                                               // Команда запроса данных автоподсветки
      EEPROM.get(ST_TIME_ADDR,   st_time);                                                         // Парсим данные из EEPROM...
      EEPROM.get(END_TIME_ADDR, end_time);                                                         // ...
      EEPROM.get(ILLUM_ADDR,       illum);                                                         // ...
      Serial.print(String(st_time) + "," + String(end_time) + "," + String(illum));                // Печатаем в серийный порт
    }

    if (val == 20) { Serial.print(ee_read(SSID_ADDR) + "|" + ee_read(PASS_ADDR) + "|"); }          // Команда запроса параметров сети WiFi
  }
}



  
void set_wifi() {                                         // Функция отвечает за установку нового WiFi на устройстве
  POS_3 = 0;                                              // Позиция начальной горизонтальной ориентации

  byte len2, len1;                                        // Длина имени и пароля сети WiFi соответственно

  flag = false;                                           // Поднятый флаг - выход из раздела 
  lcd.clear(); lcd.cursor();                              // Оищаем вывод LCDВклюаем подчеркивание у курсора
  lcd.setCursor(0, 0); lcd.print("SSID (Len 00)");        // Печатаем данные на LCD
  lcd.setCursor(0, 2); lcd.print("PASS (Len 00)");        // Печатаем данные на LCD
  
  len1 = set_len(0);                                      // Запускаем фукнцию установки длины имени сети
  if (not flag and len1) ssID = set_string(0, len1);      // Запускаем фукнцию установки имени сети
  
  if (not flag) len2 = set_len(2);                        // Запускаем фукнцию установки длины пароля сети
  if (not flag and len2) PASS = set_string(2, len2);      // Запускаем фукнцию установки пароля сети
 
  if (not flag and ssID and PASS) {                       // Если не был активирован выход из разднла и имя и пароль не нулевые
    ee_write(SSID_ADDR, ssID); ee_write(PASS_ADDR, PASS); // Кладём данные в EEPROM
  }
  Serial.write(20);                                       // Печатаем упраляющий символ - установку новой сети на NodeMCU
  Serial.println(ssID); Serial.println(PASS);             // Печатаем имя и пароль сети
  
  lcd.noCursor(); 
  if (screenState) rendering(DATA_SETT, POS_2);            // Отключаем курсор; отрисовываем настройки
  ssID = ""; PASS = "";                                    // "Обнуляем" переменные 
}



String ee_read(int STR_ADDR) {                            // Функция чтения строки из EEPROM
  String str;                                             // Создаём строку 
  int len = EEPROM.read(STR_ADDR);                        // Читаем длину строки
  str.reserve(len);                                       // Резервируем место (для оптимальности)
  
  for (int i = 0; i < len; i++) {                         // Читаем строку побайтово...
    str += (char)EEPROM.read(STR_ADDR + 1 + i);           // ...
  }
  return str;                                             // Возвращаем полученную строку
}

void ee_write(int STR_ADDR, String inc) {                 // Функция записи строки по указанному адресу
  int len = inc.length();                                 // Получаем длину строки
  EEPROM.write(STR_ADDR, len);                            // Записываем её по указанному адресу
  for (int i = 0; i < len; i++) {                         // И далее саму строку посимвольно...
    EEPROM.write(STR_ADDR + 1 + i, inc[i]);               // ...
  }
}

String set_string(byte row, byte len) {                   // Функция установки нового имени и пароля сети
  char arr[len]; pos = 0; bool f = false;                 // Создаём вспомогательные переменные                          
  
  lcd.setCursor(0, row + 1);                              // Ставим курсор на первый символ
    
  while (true) {                                          // Запускаем условно бесконечный цикл
    but.tick(); 
    if (but.isPress()) {                                  // Если кнопка нажата...
      btn_for_lcd(); flag = true; break;                  // ...то обрабвтываем данное событие
    }
    if (millis() - time_off > time_lcd_is_on) {           // Если экран горит больше установленного времени...
      screenState = false; lcd.clear(); POS = 0;          // ...то выключаем его
      lcd.noBacklight(); flag = true; break;              // Выходим из раздела
    }
    enc.tick();                                           // Обязательная функция обработки энкодера (должна постоянно опрашиваться)
    if (enc.isPress()) {                                  // Если кнопка на энкодере была нажата
      time_off = millis();
      if (f) arr[POS_3] = char(pgm_read_byte(&str[pos])); // Если текущая позиция не пуста
      pos = 0;                                            // Текущий номер буквы в массиве str
      POS_3 += 1; POS_3 %= len;                           // Увеличиваем текущую позицию - смещаемся вправо, если край, то попадаем в начало
      lcd.setCursor(POS_3, row + 1); f = false;           // Переводим курсор на следущую ячейку
    }
    if (enc.isHolded() or flag) {                         // Если кнопка была длительно нажата...
      time_off = millis(); break;                         // ...то выходим из раздела
    }                    
   
    if (enc.isRight()) {                                  // Если поворот вправо, увеличиваем значение в выбранной ячейке
      time_off = millis();pos += 1; pos %= 67; f = true;  // Увеличиваем на один значение в текущей ячейке; делаем проверку корректности
      arr[POS_3] = char(pgm_read_byte(&str[pos]));        // n-я буква имени/пароля равна pos-той в массиве str
      lcd.setCursor(POS_3, row + 1);                      // Устанавливаем курсор на текущую ячейку
      lcd.print(arr[POS_3]);                              // Выводим изменившиеся значения в LCD
      lcd.setCursor(POS_3, row + 1);                      // Устанавливаем курсор на текущую ячейку
    } 
    if (enc.isLeft()) {                                   // Если поворот вправо, уменьшаем значение в выбранной ячейке
      pos -= 1; if (pos == 255) pos = 66; f = true;       // Увеличиваем на один значение в текущей ячейке; делаем проверку корректности
      arr[POS_3] = char(pgm_read_byte(&str[pos]));        // n-я буква имени/пароля равна pos-той в массиве str
      lcd.setCursor(POS_3, row + 1);                      // Устанавливаем курсор на текущую ячейку
      lcd.print(arr[POS_3]); time_off = millis();         // Выводим изменившиеся значения в LCD
      lcd.setCursor(POS_3, row + 1);                      // Устанавливаем курсор на текущую ячейку
    }
  }
  String res = "";                                        // Создаём строку-ответ
  for (byte i = 0; i < len; i++) {                        // Читаем её посимвольно из массива arr...
    res += arr[i];                                        // ...
  } 
  return res;                                             // Возвращаем результат
}

byte set_len(byte row) {                                  // Функция установки длины имени/пароля сети
  lcd.setCursor(11, row);                                 // Устанавливаем курсор на 12 ячейку в строке row
  byte lens = 0;                                          // Создаём длину-ответ
  while (true) {                                          // Запускаем условно бесконечный цикл
    but.tick();                                           // Функция опроса кнопки включения/выключения экрана (должна постоянно опрашиваться)
    if (but.isPress()) {                                  // Если кнопка нажата...
      btn_for_lcd(); flag = true; break;                  // ...то обрабатываем данное событие
    }
    if (millis() - time_off > time_lcd_is_on) {           // Если экран горит больше установленного времени...
      screenState = false; lcd.clear(); POS = 0;          // ...то выключаем его
      lcd.noBacklight(); flag = true; break;              // Выходим из раздела
    }
    enc.tick();                                           // Обязательная функция обработки энкодера (должна постоянно опрашиваться)
    if (enc.isRight()) {                                  // Если ручка энкодера была повёрнута вправо...
      time_off = millis();
      lens += 1; if (lens > 20) lens = 20;              lcd.setCursor(10, row);            // Увеличиваем длину, так, чтобы не была больше 20
      lcd.print(String(lens / 10) + String(lens % 10)); lcd.setCursor(lens - 1,  row + 1); // Печатаем изменившиеся данные в LCD
      lcd.print("#");                                   lcd.setCursor(11, row);            // Отображаем изменившуюся длину графически
    }
    if (enc.isLeft()) {                                                                    // Если ручка энкодера была повёрнута влево...
      time_off = millis();
      lens -= 1; if (lens == 255) lens = 0;             lcd.setCursor(10, row);            // Уменьшаем длину, так, чтобы не была меньше 0
      lcd.print(String(lens / 10) + String(lens % 10)); lcd.setCursor(lens, row + 1);      // Печатаем изменившиеся данные в LCD
      lcd.print(" ");                                   lcd.setCursor(11, row);            // Отображаем изменившуюся длину графически
    }
    if (enc.isPress()) {                                  // Если кнопка энкодера была нажата...
      time_off = millis(); delay(1000); enc.tick();       // Задержка 1с, опрос кнопки энкодера 
      if (enc.isHolded()) { flag = true; break; }         // Если она была нажата, то выходим из разднла установвки нового WiFi
      break;                                              // Выходим из цикла и переходим к установке имени или пароля сети
    }
  }
  return lens;                                            // Возвращаем полученную длину
}

void rendering(String ARRAY[], int pos) {                 // Функция выводит текст в LCD
  lcd.clear();                                            // Очищаем экран ЖК-дисплея
  lcd.setCursor(0, 0);                                    // Устанавливаем курсор в начало первой строки
  lcd.print(ARRAY[pos]);                                  // Выводим на первую строку LCD первый элемент массива ARRAY
  lcd.setCursor(0, 1);                                    // Устанавливаем курсор в начало второй строки
  lcd.print(ARRAY[pos + 1]);                              // Выводим на вторую строку LCD второй элемент массива ARRAY
  lcd.setCursor(0, 2);                                    // Устанавливаем курсор в начало третьей строки
  lcd.print(ARRAY[pos + 2]);                              // Выводим на третью строку LCD третий элемент массива ARRAY
  lcd.setCursor(0, 3);                                    // Устанавливаем курсор в начало четвёртой строки
  lcd.print(ARRAY[pos + 3]);                              // Выводим на четвёртую строку LCD четвёртый элемент массива ARRAY
  lcd.setCursor(19, 3);                                   // Устанавливаем курсор в конец четвёртой строки
  lcd.write(0);                                           // Символ "курсора" (byte customChar[] = { B00000, B00100, B01110, B11111, B11111, B01110, B00100, B00000 })
}

int correct_pos_and_rendering(String ARRAY[], int pos, int len) {                     // Функция проверяет корректность перемнной POS, а затем выводит данные в LCD 
  if (pos < 0) pos = 0;                                                               // Отсекаются значения меньшие 0  (так как индексация в массиве DATA_SCREEN начинается с 0)
  else if (pos > len - 4) pos = len - 4;                                              // Отсекаются значения превышающие <длина DADA_SCREEN> - 4 (так как POS + 3 должно не превышать индекс последнего элмента в массиве DATA_SCREEN) 
  else  rendering(ARRAY, pos);                                                        // Если не вышли за рамки, то отбражаем данные на экран
  return pos;                                                                         // Возвращаем полученную позицию
}

int rendering_after_rotation(String ARRAY[], int pos, int len) {                      // Функция отображает данные после поворота ручки энкодера
    if (enc.isRight()) {                                                              // Если был поворот энкодера вправо
      pos += 1; pos = correct_pos_and_rendering(ARRAY, pos, len);                     // Увеличиваем POS на 1 и вызываем функцию correct_pos_and_rendering()
    }
    else if (enc.isLeft()) {                                                          // Если был поворот энкодера влево
      pos -= 1; pos = correct_pos_and_rendering(ARRAY, pos, len);                     // Уменьшаем POS на 1 и вызываем функцию correct_pos_and_rendering()
    }
    return pos;                                                                       // Возвращаем полученную позицию
}

void reading_sensor_values() {                                                        // Функция считывания показаний с датчиков
  val_pot = map(analogRead(POT), 0, 1023, 0, 100);                                    // Считываем значения фоторезистора и конвертируем в проценты
  DATA_SCREEN[2] = ((String) "Luminosity:  " + val_pot + "%");                        // Обновление массива главного экрана
  
  sensor.requestTemp();                                                               // Считываем температуру
  if (sensor.readTemp()) {                                                            // Если значения получены корректные...
    val_temp = sensor.getTemp();                                                      // Обновляем значение текущей температуры
    DATA_SCREEN[1] = ((String) "Temperature: " + int(val_temp) + "*C");               // Обновляем массив для отрисовки в LCD
  }
  else DATA_SCREEN[3] = DATA_SCREEN[1] = ("DS18B20: HET OTBETA");                     // Обновление массива главного экрана
}

void reading_soil_hum() {                                                             // Функция чтения влажности почвы
  digitalWrite(VCC_POT, HIGH); delay(150);                                            // Подаём питание на датчик, ждём начало его работы
  val_sensor = analogRead(SENSOR);                                                    // Читаем сырые данные с датчика влажности почвы (диапозон 0-1024)
  val_sensor = map(val_sensor, MIN, MAX, 0, 100);                                     // адаптируем значения от 0 до 100
  val_sensor = constrain(val_sensor, 0, 100);                                         // убираем значения, вышедшие за пределы
  DATA_SCREEN[3] = ((String) "Soil Hum.:   " + val_sensor + "%");                     // Обновление массива главного экрана
  digitalWrite(VCC_POT, LOW);                                                         // Выключаем питание
}

void reading_time() {
  currTime = rtc.getTime();                                                           // Текущее время равно счианному
  DateTime now = rtc.getTime();                                                       // Переменая, хранящая текущее время для последущего "вычленения"
  if (!rtc.begin()) {                                                                 // Проверка подключен ли модуль часов реального времени
    DATA_SCREEN[0] = "DS3231 not found";                                              // Если нет, то программа выводит данную строчку 
  } else {
    DATA_SCREEN[0] =  String(now.date / 10) +  String(now.date % 10) + ".";           // Обновляем массив для вывода в LCD...
    DATA_SCREEN[0] += String(now.month / 10) + String(now.month % 10) + ".";          // ...
    DATA_SCREEN[0] += String(now.year) + "   " + String(now.hour / 10);               // ...
    DATA_SCREEN[0] += String(now.hour % 10) + ":" + String(byte(now.minute) / 10);    // ...
    DATA_SCREEN[0] += String(byte(now.minute) % 10);                                  // ...
  }
}

void water_level() {                                                                  // Функция проверки уровня воды
  digitalWrite(VCC_LEV, HIGH);                                                        // Подаём питание на датчик
  delay(50);                                                                          // Задержка, чтобы он пришёл в активное состояние
  level = digitalRead(LEVEL);                                                         // Считываем значение
  digitalWrite(VCC_LEV, LOW);                                                         // Отключаем питание
   
  if (level) { Serial.write(36); digitalWrite(LED, HIGH); }                           // Если воды нет, то передаём сигнал на плату NodeMCU и зажигаем светодиод
  else             digitalWrite(LED, LOW);                                            // Иначе гасим светодиод
}
