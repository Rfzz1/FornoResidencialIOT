#include <Arduino.h>
#include "config.h"
#include "utils.h"
#include "sessao.h"
#include "telemetria.h"
#include "api.h"

static unsigned long milisInicioSessao = 0;
static unsigned long tempoLigado = 0;
static unsigned long milisUltimaAtualizacao = 0;
static uint32_t ultimoSegundoExibido = 0;

// Cuida só do cronômetro da sessão (tempo ligado).
// Antes esta task TAMBÉM calculava o estado do forno (definirEstadoForno) e
// gravava em condicoes, ao mesmo tempo que a taskCerebro, em outro núcleo.
// Agora só a taskCerebro calcula o estado; aqui apenas lemos.
void taskSessao(void *parameter)
{
  estadoForno estadoAnteriorLido = FORNO_DESLIGADO;

  for (;;)
  {
    xSemaphoreTake(mutexEstadoForno, portMAX_DELAY);
      estadoForno estadoAtualLido = condicoes.estadoFornoAtual;
    xSemaphoreGive(mutexEstadoForno);

    tratarInicioSessao(estadoAtualLido, estadoAnteriorLido);
    tratarFimSessao(estadoAtualLido, estadoAnteriorLido);

    estadoAnteriorLido = estadoAtualLido;

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void tratarInicioSessao(estadoForno estadoFornoAtual, estadoForno estadoFornoAnterior) {
  if (estadoFornoAtual != FORNO_DESLIGADO &&
      estadoFornoAnterior == FORNO_DESLIGADO) {
    
    milisInicioSessao = millis();
    Serial.println("Sessao iniciada!");
  }
}

void tratarFimSessao(estadoForno estadoFornoAtual, estadoForno estadoFornoAnterior) {
  if (estadoFornoAnterior != FORNO_DESLIGADO &&
      estadoFornoAtual == FORNO_DESLIGADO) {
    Serial.println("Sessao encerrada!");
    Serial.print("Tempo total ligado: ");
    Serial.print(condicoes.tempoLigadoSegundos);
    Serial.println(" segundos");
    tempoLigado = 0;

    xSemaphoreTake(mutexTelemetria, portMAX_DELAY);
    
      condicoes.tempoLigadoSegundos = 0;

    xSemaphoreGive(mutexTelemetria);
  }
}

void tratarTempoSessao() {

  // Agora sim podemos usar o return com segurança, DEPOIS de verificar se ele acabou de desligar.

  if (condicoes.estadoFornoAtual == FORNO_DESLIGADO)
  {
    return;
  }

  // 4. Se chegou até aqui, o forno ESTÁ LIGADO. Atualiza o cronômetro.

  tempoLigado = millis() - milisInicioSessao;

  xSemaphoreTake(mutexTelemetria, portMAX_DELAY);

    condicoes.tempoLigadoSegundos = tempoLigado / 1000;
    condicoes.tempoLigadoMinutos = condicoes.tempoLigadoSegundos / 60;
    condicoes.tempoLigadoHoras = condicoes.tempoLigadoMinutos / 60;

  xSemaphoreGive(mutexTelemetria);

  if (condicoes.tempoLigadoSegundos != ultimoSegundoExibido)
  {

    ultimoSegundoExibido = condicoes.tempoLigadoSegundos;
    Serial.print("Tempo ligado: ");
    Serial.print(condicoes.tempoLigadoSegundos);
    Serial.println(" segundos");

    if (millis() - milisUltimaAtualizacao >= 10000)
    {

      milisUltimaAtualizacao = millis();
      atualizarSessao();
    }
  }
}
