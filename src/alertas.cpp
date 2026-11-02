#include <Arduino.h>
#include "config.h"
#include "alertas.h"
#include "telemetria.h"
#include "buzzer.h"
#include "leds.h"

void taskAlertas(void *parameter)
{

  estadoForno estadoFornoAtual;
  estadoSistema estadoSistemaAtual;
  estadoSistema estadoAnterior = SEGURO;
  bool buzzerMutado;
  bool temporizadorLigado;
  unsigned long inicioDisparo;

  for (;;)
  {

    bool pularBuzzerPadrao = false;

    xSemaphoreTake(mutexEstadoForno, portMAX_DELAY);
    estadoFornoAtual = condicoes.estadoFornoAtual;

    xSemaphoreGive(mutexEstadoForno);

    xSemaphoreTake(mutexEstadoSistema, portMAX_DELAY);
    estadoSistemaAtual = condicoes.estadoAtual;

    xSemaphoreGive(mutexEstadoSistema);

    xSemaphoreTake(mutexWebSocket, portMAX_DELAY);
    buzzerMutado = condicoes.buzzerMutado;

    xSemaphoreGive(mutexWebSocket);

    xSemaphoreTake(mutexTemporizador, portMAX_DELAY);
    temporizadorLigado = condicoes.temporizadorLigado;
    inicioDisparo = condicoes.inicioDisparo;

    xSemaphoreGive(mutexTemporizador);

    if (estadoAnterior != estadoSistemaAtual)
    {

      if ((estadoAnterior == CRITICO || estadoAnterior == ALERTA || estadoAnterior == ERRO_SENSOR) && estadoSistemaAtual == SEGURO)
      {

        xSemaphoreTake(mutexWebSocket, portMAX_DELAY);
        condicoes.buzzerMutado = false;
        xSemaphoreGive(mutexWebSocket);
      }
    }

    estadoAnterior = estadoSistemaAtual;

    if (buzzerMutado)
    {
      desligarBuzzer();
      pularBuzzerPadrao = true;
    }

    if (temporizadorLigado)
    {

      if (!buzzerMutado && (millis() - inicioDisparo < (DURACAO_APITO_TEMPORIZADOR_SEG * 1000)))
      {
        dispararBuzzer();
        pularBuzzerPadrao = true;
      }
      else
      {
        desligarBuzzer();
        xSemaphoreTake(mutexTemporizador, portMAX_DELAY);
          condicoes.temporizadorLigado = false;
        xSemaphoreGive(mutexTemporizador);
      }
    }

    alertas();
    if (!pularBuzzerPadrao)
    {
      atualizarBuzzer(estadoSistemaAtual);
    }
    atualizarLEDs(estadoSistemaAtual);

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void alertas()
{

  if (condicoes.tempoLigadoMinutos >= 90 &&
      condicoes.TEMP_ATUAL >= 200)
  {

    Serial.println("Notificacao: Talvez voce tenha esquecido sua comida no forno!");
  }
}