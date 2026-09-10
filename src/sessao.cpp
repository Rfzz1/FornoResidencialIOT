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

void taskSessao(void *parameter) {

  static estadoForno estadoFornoAnterior = dados.estadoFornoAnterior;
  static estadoForno estadoFornoAtual = dados.estadoFornoAtual;

  for (;;) {
      tratarInicioSessao(estadoFornoAtual, estadoFornoAnterior);
      tratarFimSessao(estadoFornoAtual, estadoFornoAnterior);
      tratarTempoSessao();

      estadoFornoAnterior = estadoFornoAtual;
      estadoFornoAtual = dados.estadoFornoAtual;

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
    Serial.print(dados.tempoLigadoSegundos);
    Serial.println(" segundos");

    tempoLigado = 0;
    dados.tempoLigadoSegundos = 0;
  }
}

void tratarTempoSessao() {

  // 3. Se o forno estiver desligado, não precisamos contar o tempo. 
  // Agora sim podemos usar o return com segurança, DEPOIS de verificar se ele acabou de desligar.
  if (dados.estadoFornoAtual == FORNO_DESLIGADO) {
    return;
  }

  // 4. Se chegou até aqui, o forno ESTÁ LIGADO. Atualiza o cronômetro.
  tempoLigado = millis() - milisInicioSessao;
  
  dados.tempoLigadoSegundos = tempoLigado / 1000;
  dados.tempoLigadoMinutos = dados.tempoLigadoSegundos / 60;
  dados.tempoLigadoHoras = dados.tempoLigadoMinutos / 60;

  if (dados.tempoLigadoSegundos != ultimoSegundoExibido) {
    ultimoSegundoExibido = dados.tempoLigadoSegundos;

    Serial.print("Tempo ligado: ");
    Serial.print(dados.tempoLigadoSegundos);
    Serial.println(" segundos");

    if (millis() - milisUltimaAtualizacao >= 10000) {
      milisUltimaAtualizacao = millis();
      atualizarSessao();
    }
  }
}