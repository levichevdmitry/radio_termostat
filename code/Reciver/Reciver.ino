#include <SPI.h>
#include <RH_RF22.h>
#include <stdio.h>
#include <stdlib.h>
#include <sdelay.h>

#define RF22_SDN_PIN  9
#define LED_PIN       A0
#define DC_DC_EN_PIN  A3
#define RELAY_C1_PIN  A1
#define RELAY_C2_PIN  A2

#define SLEEP_TIME        58000              // sleep time - 55 sec
#define TIME_ACTIVE_WAIT  70000              // active wait 70 sec 
#define WAKING_UP_TIME    65
#define BAT_MIN_LVL       2.0     // min battery voltage

RH_RF22 rf22;
unsigned char relay_old_state = 0;
float batteryVoltage;
unsigned long time_now, time_old;

void setup() {
  time_old = 0;
  init_hw();
}

void loop() {
  char batteryVoltageString[8] = "";
  batteryVoltage = getBandgap();
  dtostrf(batteryVoltage, 3, 2, batteryVoltageString);

  time_now = millis();
  
  if (rf22.available())  {
    digitalWrite(LED_PIN, HIGH);
    uint8_t buf[RH_RF22_MAX_MESSAGE_LEN];   // буфер
    uint8_t len = sizeof(buf);              // вычесляем размер буфера
    rf22.recv(buf, &len);

    Serial.print("buf: ");                // отправляем в Serial содержимое буфера
    Serial.println((char*)buf);

    Serial.print("RSSI: ");             // отправляем уровень сигнала последнего пакета
    Serial.println(rf22.lastRssi(), DEC);

    Serial.print("Battery: ");                // отправляем в Serial содержимое буфера
    Serial.println(batteryVoltageString);

    digitalWrite(DC_DC_EN_PIN, HIGH); // enable dc-dc converter
    delay(100);
    if (buf[2] == '1' && relay_old_state == 0) {
      digitalWrite(RELAY_C2_PIN, HIGH);
      relay_old_state = 1;
    } else if (buf[2] == '0' && relay_old_state == 1) {
      digitalWrite(RELAY_C1_PIN, HIGH);
      relay_old_state = 0;
    }
    delay(20);
    digitalWrite(RELAY_C1_PIN, LOW);
    digitalWrite(RELAY_C2_PIN, LOW);
    digitalWrite(DC_DC_EN_PIN, LOW); // disable dc-dc converter
    digitalWrite(LED_PIN, LOW);
    sleepNow();
    init_hw();
    time_old = millis();
  } else {
    if (time_now - time_old >= TIME_ACTIVE_WAIT) {
      Serial.print("No packet recived. Go sleep."); 
      sleepNow();
      init_hw();
      time_old = millis();
    }
  }

}

void init_hw() {
  Serial.begin(9600);
  pinMode(RF22_SDN_PIN, OUTPUT);
  digitalWrite(RF22_SDN_PIN, LOW);
  pinMode(LED_PIN, OUTPUT);
  pinMode(DC_DC_EN_PIN, OUTPUT);
  digitalWrite(DC_DC_EN_PIN, LOW);
  pinMode(RELAY_C1_PIN, OUTPUT);
  pinMode(RELAY_C2_PIN, OUTPUT);

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

  rf22.setTxPower(RH_RF22_TXPOW_5DBM);
  rf22.setFrequency(446.0);
  rf22.setModemConfig(RH_RF22::GFSK_Rb2_4Fd36);

}

void sleepNow()
{
  digitalWrite(LED_PIN, LOW); // led off
  //digitalWrite(RF22_SDN_PIN,HIGH); // shutdown SI4432 ?
  digitalWrite(DC_DC_EN_PIN, LOW); // disable dc-dc converter
  digitalWrite(RELAY_C1_PIN, LOW);
  digitalWrite(RELAY_C2_PIN, LOW);
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
