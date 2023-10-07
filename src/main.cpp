#include <Arduino.h>

#include <Wire.h>
#include <OneWire.h>
#include <GyverNTC.h>

// #include <DallasTemperature.h>

#define compr  5    // реле компрессора
#define fanPin  6    // пин вентилятора
#define releKN  7    // Электромагнит
#define LedT  13    // 

// **************************

// термистор на пине А1 - А3
// сопротивление резистора 10к
// тепловой коэффициент 3950
GyverNTC therm1(0, 10000, 3950);  // морозилка
GyverNTC therm2(1, 10000, 3950);  //  холодилка
GyverNTC therm3(2, 10000, 3950);  //  испарилка (испарение)

// **************************

const float minTempMor = -18;                 // Минимальна+я температура морозилки 
const float nomTempMor = -15;                 // Рабочая температура морозилки
const float maxTempMor = -12;                 // Высокая температура
const float minTempHol = 5;                   // Минимальная температура холодильника
const float nomTempHol = 8;                   // Номинальная температура холодильника
const float maxTempHol = 12;                   // Максимальная температура холодильника
const float minTempIsp = -24;                   // Минимальная температура испарителя
const float nomTempIsp = 0;                   // Номинальная температура испарителя
const float maxTempIsp = 9;                   // Максимальная температура испарителя
const float targetTemp = -18;
const float targetDefrostTemp = 10;           //Установленная температура разморозки 
const unsigned long compReadTime = 60000;     //Интервал считывания температуры компрессора 
const unsigned long heatReadTime = 10000;     //Интервал считывания температуры вентилятора
const unsigned long compWorkTime = 3000000;   //Максимальное время непрерывной работы компрессора 
const unsigned long fenWorkTime = 900000;    //Максимальное время непрерывной работы вентилятора 
const unsigned long compRestTime = 420000;    //Время паузы на охлаждение компрессора (сделать 15 минут)
const unsigned long defrostTime = 43200000;   //Интервал запуска цикла разморозки (??????????????????)
//const unsigned long fenWorkTime = 900000;
//Переменные
unsigned long lastDefrostTime;
unsigned long timing, timing1;
// **************************
boolean isEmagnUp = true; // Проверка переключения электромагнита
boolean isCheckingPass = true;  //Проверка /Старые/00000 ?????
boolean comp = false;
boolean kn = false;
boolean isFan = false;

int t1 = 0;
int t2 = 0;
int t3 = 0;
int knPin = 2;
int prPin = 4;
int meta = 5;
// int comprPin = 5;  // Компрессор
// int fanPin = 6;    // Вентилятор
int inPin = 8;     //
// int ledPin = 7;    //  Эл. магнит
int DoorOld = 0;
int DoorNew = 0;
float tmor;
float thol;
float tisp;
float tempCompr;
// float tMin = -12;
// float tIspMin = -23;
// float tIspMax = 8;
// float tMorMin = -15;
// float tMorMax = -12;
// float tHolMin = 3; // ~5
// float tHolMax = 6; // ~8
//unsigned long timing; // Переменная для хранения точки отсчета
static uint8_t tog = 0;
static uint32_t oldtime = millis();

//************** -Проверка температуры по всем датчикам***************************
float getTempAdd (){
  therm1.getTempAverage();
  therm2.getTempAverage();
  therm3.getTempAverage(); 
  tmor = therm1.getTempAverage();
  thol = therm2.getTempAverage();
  tisp = therm3.getTempAverage();
}
// ************ -Функции температуры- **************
float termCheck () { //float , tempCompr
  if (tmor <= minTempMor && thol <= minTempHol && tisp <= minTempIsp) {
      tempCompr = targetTemp;
  }
}

//Функция снятия температуры морозилки
float getTempMor(){
  //Считка данных с термодатчика
  therm1.getTempAverage();
  tmor = therm1.getTempAverage();
  float tempTemp=termCheck ();
  Serial.println(tempTemp);
  return tempTemp;
}

// ************* -Проверка температуры по отдельным датчикам- *************

float morthermProv() {
  therm1.getTempAverage();
  tmor = therm1.getTempAverage();
}
float holthermProv() {
  therm2.getTempAverage();
  thol = therm2.getTempAverage();
}
// проверка испарилки
float ispthermProv() { 
  therm3.getTempAverage();
  tisp = therm3.getTempAverage();
}
//*********************************************
/*void cycleMorMin(){
  if ((isEmagnUp == false && tisp > 0 )|| (isEmagnUp == true && tisp <0)){
    fanCycle ();              //доделать********************************************!!!
  } else if (isEmagnUp == true && tisp > 0){
}*/

// **************************
// Цикл проверки испарителя 5 переходов
void ispProvTemp () {
  ispthermProv ();  // добавлено 21.09.23
  float tisp2 = tisp; //добавлено 21.09.23
  delay(1000); //добавлено 21.09.23
   for (int i = 0; i <= 5; i++ ) { 
    ispthermProv ();
    Serial.println (tisp);
    delay(30000);  
  } 
  if (tisp > tisp2) { isEmagnUp = true;}//добавлено 21.09.23
  else if (tisp < tisp2) {isEmagnUp = false;} //добавлено 21.09.23
  
}
// **************************
// Проверка испарителя
void ProvTisp() {
    //int meta = 6;
  ispthermProv(); //oct

  t2 = tisp;   // oct
  delay(30000); // oct
ispProvTemp ();

  Serial.print("PrvTisp END ");
  Serial.println(tisp);
  delay(100);
  if (t2 < tisp) {
    Serial.print("t2 tisp < ");
    Serial.println(tisp);
  } else Serial.println("t2 tisp !< ");
  therm3.getTempAverage();
  tisp = therm3.getTempAverage();
  delay(100);
  t3 = tisp;
  delay(30000);
   for (int i = 0; i <= 6; i++ ) {
    ispthermProv ();
    Serial.println (tisp);
    delay(30000);
  }

  Serial.print("PrvTisp 2 END ");
  Serial.println(tisp);
  delay(100);
  // Serial.print("PrvTisp 2 ");
  // Serial.println(tisp);
  // delay (180000); // подрегулировать нужно --------------------!
  // therm3.getTempAverage();
  // tisp = therm3.getTempAverage();
  if (t3 < tisp) {
    Serial.print("t3 tisp < ");
    Serial.println(tisp);
  } else Serial.println("t3 tisp !< ");
  delay(100);
  if (t3 && t2 < tisp) {
    Serial.print(" OH YES ! MagnUP true - Tepleet ! ");
    Serial.println(tisp);
    t3 = tisp;
    isEmagnUp = true;
    Serial.println("t3 & t2 tisp < ");
  } else {
    Serial.print(" OH NO ! MagnDown false - Holodaet ! ");
    t2 = tisp;
    t3 = tisp;
    isEmagnUp = false;
    Serial.println("t3 & t2 tisp !< ");
  }
  isCheckingPass = true;
  //float toStrl (tmor, thol, tisp){
}
//------------------------------------------------------//
// Включение клапана
void klapan() {
  digitalWrite(releKN, HIGH);
  delay(1);
  digitalWrite(releKN, LOW);  //digitalWrite(7, LOW);
}
// **************************
// ************* -Подпрограмма цикла вентилятора- ******************
void fanCycle() {
  unsigned long heatTimeStamp=millis(); //Время включения вентилятора
  digitalWrite (LedT, HIGH);            //Включение индикатора вентилятора
  while (millis() - heatTimeStamp < fenWorkTime) //Проверка на допустимое время работы *****(ссылка на цикл)*****
    {if ((ispthermProv() <= minTempIsp && isEmagnUp == true) // Переделать ******** - ????????????????
    || (ispthermProv() >= maxTempIsp && isEmagnUp == false)) //Проверка на температуру getTemp() < targetDefrostTempтемпературу испарителя !!!!!!!!!!!!!!
      {digitalWrite (fanPin, HIGH);}     //Включение вентилятора
    else {digitalWrite (fanPin, LOW);};  //Выключение вентилятора
  delay (heatReadTime);                 //Интервал проверки температуры и времени
  }
  // время паузы вентилятора 5 минут____________
  
  digitalWrite (fanPin, LOW);            //Последнее выключение вентилятора ??????????????
  digitalWrite (LedT, LOW);             //Выключение индикатора вентилятора ???????????????
  delay(300000);
}
// *****************************************
void cycleMorMin() { 
  unsigned long morTimeStamp=millis(); //Время включения вентилятора
  //----- если тisp больше 0 и емагн увеличивается, тогда - клапан ispthermProv() 
  // ----иначе вкл. цикл фена и ждём температуру 0 градусов после чего отключаем фен morthermProv()
  //---- и ждем температуру испарителя -24 градуса ProvTisp()
// Если температура морозилки больше максимальной и время меньше установленного
   while ( tmor > maxTempMor && millis() - morTimeStamp < fenWorkTime ) {
if (tisp > 0 && isEmagnUp == true){
  klapan(); 
  ProvTisp(); //ispProvTemp ---!!!!!!!!!!!!!!!!!!!!!

  while (tisp > -23) { 
    ispthermProv();  
  }
} else if (tisp >0 && isEmagnUp == false) {
  while (tisp > -23) {
  ispthermProv();
  }
}
klapan ();
digitalWrite (LedT, HIGH);
digitalWrite (fanPin, HIGH);
morthermProv();
delay (heatReadTime);
   }
   digitalWrite (LedT, HIGH);
   digitalWrite (fanPin, LOW);
}
//*********** -Подпрограмма цикла компрессии- *****************/
 void compressorCycle() {
  unsigned long compTimeStamp=millis(); //Время включения компрессора
  while (getTempMor() > targetTemp && millis() - compTimeStamp < compWorkTime) //Проверка на температуру и допустимое время работы
  // Нужно напрямую ссылаться на функцию
    {digitalWrite(compr, HIGH); //Включение компрессора
  if (!isCheckingPass) {
    ProvTisp(); } // начать проверку температуры испарителя
  if (tmor < maxTempMor){
    cycleMorMin();
  }


    delay (compReadTime);       //Интервал проверки температуры и времени
    }
  digitalWrite(compr, LOW);     //Выключение компрессора
  delay (compRestTime);         //Принудительная пауза перед перезапуском компрессора
  }
void setup() { 
  //Инициализация реле компрессора
  pinMode(compr, OUTPUT);
  digitalWrite(compr, LOW);
  //Инициализация выхода вентилятора
  pinMode(fanPin, OUTPUT);
  digitalWrite(fanPin, LOW);
  //Инициализация реле испарителя
  pinMode(releKN, OUTPUT);
  digitalWrite(releKN, LOW);
  //Инициализация индикатора цикла вентилятора
  pinMode(LedT, OUTPUT);
  digitalWrite (LedT, HIGH);
  //Инициализация термодатчика
//  TempSensors.begin();
  //Сериал-порт для дебага
  Serial.begin(9600);
  //Задержка семь минут, защита от скачков напряжения
  delay(420000);
  lastDefrostTime = millis();
  digitalWrite (LedT, LOW);
}
// **************************

// **************************
void loop() {
  getTempAdd(); // Это нужно пересмотреть ---------!!!!!!!!!!!!!!!!!!!!!!!!!
  if (tmor > nomTempMor || thol > nomTempHol ) {
   compressorCycle();                    //Цикл компрессии
  }
  delay (60000);
}  





// ************************** -пробные-***************