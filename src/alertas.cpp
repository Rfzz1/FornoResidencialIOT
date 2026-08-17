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

  for (;;) {

    xSemaphoreTake(mutexEstadoForno, portMAX_DELAY);
    estadoFornoAtual = dados.estadoFornoAtual;  

    xSemaphoreGive(mutexEstadoForno);

    xSemaphoreTake(mutexEstadoSistema, portMAX_DELAY);
    estadoSistemaAtual = dados.estadoAtual;

    xSemaphoreGive(mutexEstadoSistema);
    
    alertas();

    atualizarBuzzer();
    atualizarLEDs();
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