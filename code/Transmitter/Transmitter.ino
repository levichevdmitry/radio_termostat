#include <SPI.h>
#include <RH_RF22.h>  // http://www.airspayce.com/mikem/arduino/RF22/index.html
#include <stdio.h>
#include <stdlib.h>
#include <sdelay.h>

#define RF22_SDN_PIN  9
#define LED_PIN       A0
#define INPUT_PIN     3
#define SLEEP_TIME      60000              // sleep time - 10 sec
#define WAKING_UP_TIME  65
#define BAT_MIN_LVL     2.0     // min battery voltage

RH_RF22 rf22(10, 2);    // СS, INT .. по умолчанию 10, (D2)

float batteryVoltage;

void setup() {
  init_hw();
}

void loop() {
  init_hw();
  char data[5] = "R:";                      // заполняем массив
  char batteryVoltageString[8] = "";
  batteryVoltage = getBandgap();
  dtostrf(batteryVoltage, 3, 2, batteryVoltageString);
  if (!digitalRead(INPUT_PIN)) {
    data[2] = '1';
  } else {
    data[2] = '0';
  }
  digitalWrite(LED_PIN, HIGH);
  rf22.send(data, sizeof(data));            // отправляем
  rf22.waitPacketSent();                    // ждем пока пакет будет отправлен
  digitalWrite(LED_PIN, LOW);
  Serial.print("Send: ");
  Serial.println(data);
  Serial.print("Battery: ");                // отправляем в Serial содержимое буфера
  Serial.println(batteryVoltageString);
  sleepNow();
}

void sleepNow()
{
  digitalWrite(LED_PIN, LOW); // led off
  rf22.sleep();
  // disable ADC
  ADCSRA = 0;
  Serial.end();
  unsigned long sleepPeriod = SLEEP_TIME;
  while (sleepPeriod >= 8192 + WAKING_UP_TIME) {
    sleepWithWDT(WDTO_8S);
    sleepPeriod -= 8192 + WAKING_UP_TIME;
  }
}

void init_hw() {
  Serial.begin(9600);
  pinMode(RF22_SDN_PIN, OUTPUT);
  digitalWrite(RF22_SDN_PIN, LOW);
  pinMode(LED_PIN, OUTPUT);
  pinMode(INPUT_PIN, INPUT);

  // test battery voltage
  batteryVoltage = getBandgap();
  if (batteryVoltage < BAT_MIN_LVL) {
    Serial.println("Change battery!");
    while (1) { // say to need change battery
      digitalWrite(LED_PIN, HIGH);
      sdelay(100);
      digitalWrite(LED_PIN, LOW);
      sdelay(3000);
    }
  }

  unsigned char i = 5;
  while (i) {
    if (!rf22.init()) { // по умолчанию, мощьность 8DBM, частота 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
      Serial.println("Init failed.");
      sdelay(1000);
      Serial.println("Retry...");
    } else {
      Serial.println("Init success.");
      break;
    }
    i--;
  }

  if (i == 0) {
    digitalWrite(LED_PIN, HIGH);
    while (1) {
      digitalWrite(LED_PIN, HIGH);
      sdelay(500);
      digitalWrite(LED_PIN, LOW);
      sdelay(500);
    }
  }

  rf22.setTxPower(RH_RF22_TXPOW_14DBM); //RH_RF22_RF23B_TXPOW_14DBM
  // RH_RF22_TXPOW_1DBM
  // RH_RF22_TXPOW_2DBM
  // RH_RF22_TXPOW_5DBM
  // RH_RF22_TXPOW_8DBM
  // RH_RF22_TXPOW_11DBM
  // RH_RF22_TXPOW_14DBM
  // RH_RF22_TXPOW_17DBM
  // RH_RF22_TXPOW_20DBM

  rf22.setFrequency(446.0);                         // частота 466.0мгц, шаг 0.05мгц.
  //  rf22.setFrequency(446.0, 0.1);                  // тоже, но с автоподстройкой в 100кгц (по умолчанию 0,05)

  rf22.setModemConfig(RH_RF22::GFSK_Rb2_4Fd36);     // скорость и модуляция, Rb = 2.4kbs, Fd = 36kHz.
  // http://www.airspayce.com/mikem/arduino/RF22/classRF22.html#a76cd019f98e4f17d9ec00e54e5947ca1
}

float getBandgap ()
{
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  float data = (high << 8) | low;

  float result = 1126400 / data / 1000;
  return result; // Vcc in volts
}

ISR(WDT_vect) {} //uncomment this if your board keeps resetting, comment this out if your sketch or other libraries already define this ISR
