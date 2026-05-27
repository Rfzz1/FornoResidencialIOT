  #include <Arduino.h>
  #include "config.h"
  #include "sensores.h"
  #include "estados.h"
  #include "leds.h"
  #include "buzzer.h"
  #include "sessao.h"
  #include "alertas.h"
  #include "logs.h"
  #include "iot.h"
  #include "telemetria.h"
  #include "eventos.h"

  void setup() {

    Serial.begin(115200);
    conectarWiFi();

    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);

    pinMode(BUZZER, OUTPUT);

    dados.milisEstabilizarTermopar = millis();

  }

  void loop() {

    Blynk.run(); 

    atualizarSensores();
    atualizarEstadoSistema();
    atualizarEstadoForno();
    processarEventos();
    tratarSessao();
    atualizarAlertas();
    atualizarHorarioAlarme();
    atualizarEnvioLogs();
    atualizarEnvioBlynk();
    atualizarLEDs();
    atualizarBuzzer();

  }