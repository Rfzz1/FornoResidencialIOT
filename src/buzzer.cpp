#include <Arduino.h>
#include "config.h"
#include "buzzer.h"
#include "telemetria.h"

static unsigned long milisBuzzerErro = 0;
static unsigned long milisBuzzerAlerta = 0;
static unsigned long milisBuzzerHorario = 0;

void desligarBuzzer() {

  digitalWrite(BUZZER, LOW);

}

void buzzerHorarioAlarme() {

  if (millis() - milisBuzzerHorario >= 1000) {

    milisBuzzerHorario = millis();

    dados.buzzerAtivaHorario = !dados.buzzerAtivaHorario;

    digitalWrite(BUZZER, dados.buzzerAtivaHorario);

  }

}

void buzzerErroSensor() {

  if (millis() - milisBuzzerErro >= 500) {

    milisBuzzerErro = millis();

    dados.estadoBuzzerPisca = !dados.estadoBuzzerPisca;

    digitalWrite(BUZZER, dados.estadoBuzzerPisca);

  }
}

void buzzerAlerta() {

  if (millis() - milisBuzzerAlerta >= 2000) {

    milisBuzzerAlerta = millis();

    dados.estadoBuzzerPisca = !dados.estadoBuzzerPisca;

    digitalWrite(BUZZER, dados.estadoBuzzerPisca);

  }
}

void buzzerCritico() {

  digitalWrite(BUZZER, HIGH);

}

void atualizarBuzzer() {

  if (dados.estadoAtual != CRITICO && dados.estadoAtual != ALERTA && dados.estadoAtual != ERRO_SENSOR) {
    dados.buzzerMutado = false;
  }

  if (dados.buzzerMutado) {
    desligarBuzzer();
    return;
  }

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