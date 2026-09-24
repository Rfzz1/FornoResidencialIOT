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
  bool buzzerMutado;
  bool temporizadorLigado;
  unsigned long agora = 0;
  unsigned long mensagemChegou;
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

    xSemaphoreGive(mutexTemporizador);

    if (estadoSistemaAtual != CRITICO && estadoSistemaAtual != ALERTA && estadoSistemaAtual != ERRO_SENSOR) {
      xSemaphoreTake(mutexWebSocket, portMAX_DELAY);
        dados.buzzerMutado = false;
      xSemaphoreGive(mutexWebSocket);
    }

    if (buzzerMutado) {
      desligarBuzzer();
      continue; 
    }

    if (temporizadorLigado) {

      agora = millis();

      if ((agora - mensagemChegou)/1000 >= duracaoSegundosTemporizador) {
          dispararBuzzer();
          pularBuzzerPadrao = true;
          agora = 0;
          temporizadorLigado = false;
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