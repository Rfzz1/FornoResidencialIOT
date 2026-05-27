#include <Arduino.h>
#include "config.h"
#include "buzzer.h"
#include "telemetria.h"

static unsigned long milisBuzzerErro = 0;
static unsigned long milisBuzzerAlerta = 0;

void desligarBuzzer() {

  digitalWrite(BUZZER, LOW);

}

void buzzerErroSensor() {

  if (millis() - milisBuzzerErro >= 500) {

    milisBuzzerErro = millis();

    dados.estadoBuzzer = !dados.estadoBuzzer;

    digitalWrite(BUZZER, dados.estadoBuzzer);

  }
}

void buzzerAlerta() {

  if (millis() - milisBuzzerAlerta >= 2000) {

    milisBuzzerAlerta = millis();

    dados.estadoBuzzer = !dados.estadoBuzzer;

    digitalWrite(BUZZER, dados.estadoBuzzer);

  }
}

void buzzerCritico() {

  digitalWrite(BUZZER, HIGH);

}

void atualizarBuzzer() {

  switch (dados.estadoAtual) {

    case CRITICO:

      buzzerCritico();

      break;

    case ALERTA:

      buzzerAlerta();

      break;

    case ERRO_SENSOR:

      buzzerErroSensor();

      break;

    default:

      desligarBuzzer();

      break;
  }
}