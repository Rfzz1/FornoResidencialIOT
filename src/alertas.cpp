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
  bool entrouDisparo;
  unsigned long agora = 0;
  unsigned long mensagemChegou;
  unsigned long inicioDisparo;
  uint32_t duracaoSegundosTemporizador;

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
    mensagemChegou = condicoes.mensagemChegou;
    duracaoSegundosTemporizador = condicoes.duracaoSegundosTemporizador;
    entrouDisparo = condicoes.entrouDisparo;
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

      agora = millis();

      if ((agora - mensagemChegou) / 1000 >= duracaoSegundosTemporizador)
      {

        if (!buzzerMutado)
        {
          dispararBuzzer();
        }
        xSemaphoreTake(mutexTemporizador, portMAX_DELAY);
        condicoes.entrouDisparo = true;
        condicoes.inicioDisparo = millis();
        xSemaphoreGive(mutexTemporizador);
        pularBuzzerPadrao = true;
        agora = 0;
        xSemaphoreTake(mutexTemporizador, portMAX_DELAY);
        condicoes.temporizadorLigado = false;
        xSemaphoreGive(mutexTemporizador);
      }
      else
      {
        if (!buzzerMutado)
        {
          dispararBuzzer();
        }
        pularBuzzerPadrao = true;
      }
    }

    if (entrouDisparo)
    {

      if ((millis() - inicioDisparo) / 1000 >= 60)
      {
        desligarBuzzer();
        xSemaphoreTake(mutexTemporizador, portMAX_DELAY);
        condicoes.entrouDisparo = false;
        xSemaphoreGive(mutexTemporizador);
        pularBuzzerPadrao = true;
        inicioDisparo = 0;
      }
      else
      {
        pularBuzzerPadrao = false;
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