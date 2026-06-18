#include <Arduino.h>
#include "config.h"
#include "sessao.h"
#include "telemetria.h"
#include "api.h"

static unsigned long milisInicioSessao = 0;
static unsigned long tempoLigado = 0;
static uint32_t ultimoSegundoExibido = 0;

void tratarSessao() {

  if (dados.estadoFornoAtual != FORNO_DESLIGADO &&
      dados.estadoFornoAnterior == FORNO_DESLIGADO) {

    milisInicioSessao = millis();
    iniciarSessao();

    Serial.println("Sessao iniciada!");

  }

  if (dados.estadoFornoAtual != FORNO_DESLIGADO) {

    tempoLigado = millis() - milisInicioSessao;

    dados.tempoLigadoSegundos = tempoLigado / 1000;
    dados.tempoLigadoMinutos = dados.tempoLigadoSegundos / 60;
    dados.tempoLigadoHoras = dados.tempoLigadoMinutos / 60;

    if (dados.tempoLigadoSegundos != ultimoSegundoExibido) {

        ultimoSegundoExibido = dados.tempoLigadoSegundos;

        Serial.print("Tempo ligado: ");
        Serial.print(dados.tempoLigadoSegundos);
        Serial.println(" segundos");
    }
}

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
}