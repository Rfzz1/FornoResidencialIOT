#include <Arduino.h>
#include "config.h"
#include "buzzer.h"
#include "telemetria.h"

static unsigned long milisBuzzerErro = 0;
static unsigned long milisBuzzerAlerta = 0;

void desligarBuzzer()
{

  digitalWrite(BUZZER, LOW);
}

void buzzerErroSensor()
{

  if (millis() - milisBuzzerErro >= 500)
  {

    milisBuzzerErro = millis();

    condicoes.estadoBuzzerPisca = !condicoes.estadoBuzzerPisca;

    digitalWrite(BUZZER, condicoes.estadoBuzzerPisca);
  }
}

void buzzerAlerta()
{

  if (millis() - milisBuzzerAlerta >= 2000)
  {

    milisBuzzerAlerta = millis();

    condicoes.estadoBuzzerPisca = !condicoes.estadoBuzzerPisca;

    digitalWrite(BUZZER, condicoes.estadoBuzzerPisca);
  }
}

void buzzerCritico()
{

  digitalWrite(BUZZER, HIGH);
}

void atualizarBuzzer(estadoSistema estadoAtual)
{

  if (condicoes.estadoAtual != CRITICO && condicoes.estadoAtual != ALERTA && condicoes.estadoAtual != ERRO_SENSOR)
  {
    condicoes.buzzerMutado = false;
  }

  if (condicoes.buzzerMutado)
  {
    desligarBuzzer();
    return;
  }

  switch (estadoAtual)
  {

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