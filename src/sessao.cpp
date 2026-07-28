#include <Arduino.h>
#include "config.h"
#include "sessao.h"
#include "telemetria.h"
#include "api.h"

static unsigned long milisInicioSessao = 0;
static unsigned long tempoLigado = 0;
static unsigned long milisUltimaAtualizacao = 0;
static uint32_t ultimoSegundoExibido = 0;

void tratarSessao() {

  // 1. Detecta o INÍCIO da sessão (Ligou agora)
  if (dados.estadoFornoAtual != FORNO_DESLIGADO &&
      dados.estadoFornoAnterior == FORNO_DESLIGADO) {
    
    milisInicioSessao = millis();
    iniciarSessao();
    Serial.println("Sessao iniciada!");
  }

  // 2. Detecta o FIM da sessão (Desligou agora)
  if (dados.estadoFornoAnterior != FORNO_DESLIGADO &&
      dados.estadoFornoAtual == FORNO_DESLIGADO) {
        
    encerrarSessao();
    Serial.println("Sessao encerrada!");
    Serial.print("Tempo total ligado: ");
    Serial.print(dados.tempoLigadoSegundos);
    Serial.println(" segundos");

    tempoLigado = 0;
    dados.tempoLigadoSegundos = 0;
  }

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