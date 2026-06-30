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

    inicializarBluetooth();

    inicializarPreferences();
    verificarEstadoDispositivo();

    dados.inicioTentativa = millis(); 

    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);

    pinMode(BUZZER, OUTPUT);


    dados.milisEstabilizarTermopar = millis();

  }

  void loop() {

    verificarReiniciar();
    gerenciarEstadoOperacional();

    if (dados.espConfigurado &&
        WiFi.status() == WL_CONNECTED) {

        estabilizarHoraLocal();
    }

    atualizarLEDs();
    atualizarBuzzer();

  }