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

  for (;;)
  {

    xSemaphoreTake(mutexEstadoForno, portMAX_DELAY);
    estadoFornoAtual = condicoes.estadoFornoAtual;

    xSemaphoreGive(mutexEstadoForno);

    xSemaphoreTake(mutexEstadoSistema, portMAX_DELAY);
    estadoSistemaAtual = condicoes.estadoAtual;

    xSemaphoreGive(mutexEstadoSistema);

    alertas();
    atualizarBuzzer(estadoSistemaAtual);
    atualizarLEDs(estadoSistemaAtual);

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void alertas()
{

  condicoes.tempoLigadoHoras = condicoes.tempoLigadoSegundos / 3600.0;
  condicoes.tempoLigadoMinutos = (condicoes.tempoLigadoSegundos % 3600) / 60.0;

  if (condicoes.tempoLigadoHoras >= 1.5 &&
      condicoes.TEMP_ATUAL >= 200)
  {

    Serial.println("Notificacao: Talvez voce tenha esquecido sua comida no forno!");
  }
}