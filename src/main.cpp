  #include <Arduino.h>
  #include <ArduinoJson.h>
  #include "config.h"
  #include "sensores.h"
  #include "leds.h"
  #include "buzzer.h"
  #include "api.h"
  #include "telemetria.h"
  #include "iot.h"

  void setup() {

    Serial.begin(115200);
    verificarEstadoDispositivo();

    dados.inicioTentativa = millis();

    iniciarBlynk();  

    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);

    pinMode(BUZZER, OUTPUT);


    dados.milisEstabilizarTermopar = millis();

  }

  void loop() {

    estabilizarHoraLocal();
    conectarBlynk();

    verificarReiniciar();
    gerenciarEstadoOperacional();

    atualizarLEDs();
    atualizarBuzzer();

  }