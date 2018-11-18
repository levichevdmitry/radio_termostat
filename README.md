# Радио удлинитель сигнала термостата для газового котла
=======================

Устройство предназначенно для передачи сигнала от термостата на газовый котел по радио каналу.
Вся конструкция состоит из двух блоков - приемника и передатчика.
Работают оба блока от батареек типа AA.
Передача и прием осуществляется при помощи модулей SI4432 и контроллера ATMega328A.
Структура проекта
- bootloader - папка содержит загрузчик для 4 МГц кварца
- code  - папка содержит коды для приемника и передатчика под IDE Arduino 1.8.7
- libs - содержит библиотеки необходимые для компиляции

Схемы обоих блоков доступны на easyEDA:

 * [Приемник](https://easyeda.com/levichev.dmitry/termostat_range_extender_rx)
 * [Передатчик](https://easyeda.com/levichev.dmitry/termostat_range_extender_tx)

Ссылки:
 * [arduino-weather-station](https://github.com/tedor/arduino-weather-station)
 * [Arduino Sleep](https://github.com/cano64/ArduinoSleep)