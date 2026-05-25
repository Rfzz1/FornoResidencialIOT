#include <Arduino.h>
#include "config.h"
#include "buzzer.h"

bool estadoBuzzer = false;

unsigned long milisBuzzerErro = 0;
unsigned long milisBuzzerAlerta = 0;

void desligarBuzzer() {

  digitalWrite(BUZZER, LOW);

}

void buzzerErroSensor() {

  if (millis() - milisBuzzerErro >= 500) {

    milisBuzzerErro = millis();

    estadoBuzzer = !estadoBuzzer;

    digitalWrite(BUZZER, estadoBuzzer);

  }
}

void buzzerAlerta() {

  if (millis() - milisBuzzerAlerta >= 2000) {

    milisBuzzerAlerta = millis();

    estadoBuzzer = !estadoBuzzer;

    digitalWrite(BUZZER, estadoBuzzer);

  }
}

void buzzerCritico() {

  digitalWrite(BUZZER, HIGH);

}

void atualizarBuzzer() {

  switch (estadoAtual) {

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