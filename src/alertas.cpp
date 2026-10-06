#include <Arduino.h>
#include "config.h"
#include "alertas.h"
#include "telemetria.h"
#include "buzzer.h"
#include "leds.h"

static unsigned long milisAtualizarAlertas = 0;

void taskAlertas(void *parameter) {

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

  for (;;) {

    bool pularBuzzerPadrao = false;

    xSemaphoreTake(mutexEstadoForno, portMAX_DELAY);
    estadoFornoAtual = dados.estadoFornoAtual;  

    xSemaphoreGive(mutexEstadoForno);

    xSemaphoreTake(mutexEstadoSistema, portMAX_DELAY);
    estadoSistemaAtual = dados.estadoAtual;

    xSemaphoreGive(mutexEstadoSistema);

    xSemaphoreTake(mutexWebSocket, portMAX_DELAY);
    buzzerMutado = dados.buzzerMutado;

    xSemaphoreGive(mutexWebSocket);

    xSemaphoreTake(mutexTemporizador, portMAX_DELAY);
    temporizadorLigado = dados.temporizadorLigado;
    mensagemChegou = dados.mensagemChegou;
    duracaoSegundosTemporizador = dados.duracaoSegundosTemporizador;
    entrouDisparo = dados.entrouDisparo;
    inicioDisparo = dados.inicioDisparo;

    xSemaphoreGive(mutexTemporizador);

    if (estadoAnterior != estadoSistemaAtual) {

      if ((estadoAnterior == CRITICO || estadoAnterior == ALERTA || estadoAnterior == ERRO_SENSOR) && estadoSistemaAtual == SEGURO) {

        xSemaphoreTake(mutexWebSocket, portMAX_DELAY);
          dados.buzzerMutado = false;
        xSemaphoreGive(mutexWebSocket);
      }

    }

    estadoAnterior = estadoSistemaAtual;

    if (buzzerMutado) {
      desligarBuzzer();
      pularBuzzerPadrao = true;
    }

    if (temporizadorLigado) {

      agora = millis();

      if ((agora - mensagemChegou)/1000 >= duracaoSegundosTemporizador) {

          if (!buzzerMutado) {
             dispararBuzzer();
          }
          xSemaphoreTake(mutexTemporizador, portMAX_DELAY);
            dados.entrouDisparo = true;
            dados.inicioDisparo = millis();
          xSemaphoreGive(mutexTemporizador);
          pularBuzzerPadrao = true;
          agora = 0;
          xSemaphoreTake(mutexTemporizador, portMAX_DELAY);
            dados.temporizadorLigado = false;
          xSemaphoreGive(mutexTemporizador);
      } else {
          if (!buzzerMutado) {
             dispararBuzzer();
          }
        pularBuzzerPadrao = true;
      }

    }

    if (entrouDisparo) {

        if ((millis() - inicioDisparo)/1000 >= 60) {
          desligarBuzzer();
          xSemaphoreTake(mutexTemporizador, portMAX_DELAY);
            dados.entrouDisparo = false;
          xSemaphoreGive(mutexTemporizador);
          pularBuzzerPadrao = true;
          inicioDisparo = 0;
        } else {
          pularBuzzerPadrao = false;
        }
      }
      
    alertas();
    if (!pularBuzzerPadrao) {
      atualizarBuzzer(estadoSistemaAtual);
    }
    atualizarLEDs(estadoSistemaAtual);

    vTaskDelay(100 / portTICK_PERIOD_MS);

  }
}

void alertas() {

  dados.tempoLigadoHoras = dados.tempoLigadoSegundos / 3600.0;
  dados.tempoLigadoMinutos = (dados.tempoLigadoSegundos % 3600) / 60.0;

  if (dados.tempoLigadoHoras >= 1.5 &&
      dados.TEMP_ATUAL >= 200) {

    Serial.println("Notificacao: Talvez voce tenha esquecido sua comida no forno!");

  }

  if (dados.TEMP_EXT_ATUAL >= TEMP_EXT_MAXIMA) {
    Serial.println("Notificacao: Temperatura externa atingiu 80ºC!");
  }
}