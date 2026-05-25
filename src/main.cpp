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

void setup() {

  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  milisEstabilizarTermopar = millis();

}

void loop() {

  Blynk.run();

  if (millis() - milisEnviarBlynk >= 1000) {

      milisEnviarBlynk = millis();

      enviarDadosJSON();

  }

  if (millis() - milisEstabilizarTermopar < 2000) {

    estadoAtual = INICIANDO;

    atualizarLEDs();

    return;

  }

  lerTemperatura();

  if (!temperaturaValida()) {

    estadoAtual = ERRO_SENSOR;

  } else {

    if (estadoAtual == INICIANDO ||
        estadoAtual == ERRO_SENSOR) {

      estadoAtual = SEGURO;

    }

    definirEstadoSistema();

    definirEstadoForno();

    tratarSessao();
    alertas();

  }

  atualizarLEDs();

  atualizarBuzzer();
  enviarDadosJSON();

}