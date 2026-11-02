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

void taskSessao(void *parameter)
{

  for (;;)
  {

    tratarInicioSessao(condicoes.estadoFornoAtual, condicoes.estadoFornoAnterior);
    tratarFimSessao(condicoes.estadoFornoAtual, condicoes.estadoFornoAnterior);
    tratarTempoSessao();

    condicoes.estadoFornoAnterior = condicoes.estadoFornoAtual;
    condicoes.estadoFornoAtual = definirEstadoForno();

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
    condicoes.tempoLigadoSegundos = 0;
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

  condicoes.tempoLigadoSegundos = tempoLigado / 1000;
  condicoes.tempoLigadoMinutos = condicoes.tempoLigadoSegundos / 60;
  condicoes.tempoLigadoHoras = condicoes.tempoLigadoMinutos / 60;

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
