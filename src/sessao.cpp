#include <Arduino.h>
#include "config.h"
#include "sessao.h"

float tempoLigadoHoras = 0;

unsigned long tempoLigado = 0;
unsigned long tempoLigadoSegundos = 0;

unsigned long milisInicioSessao = 0;

void tratarSessao() {

  if (estadoFornoAtual != FORNO_DESLIGADO &&
      estadoFornoAnterior == FORNO_DESLIGADO) {

    milisInicioSessao = millis();

    Serial.println("Sessao iniciada!");

  }

  if (estadoFornoAtual != FORNO_DESLIGADO) {

    tempoLigado = millis() - milisInicioSessao;

    tempoLigadoSegundos = tempoLigado / 1000;

    Serial.print("Tempo ligado: ");
    Serial.print(tempoLigadoSegundos);
    Serial.println(" segundos");

  }

  if (estadoFornoAnterior != FORNO_DESLIGADO &&
      estadoFornoAtual == FORNO_DESLIGADO) {

    Serial.println("Sessao encerrada!");

    Serial.print("Tempo total ligado: ");
    Serial.print(tempoLigado / 1000);
    Serial.println(" segundos");

    tempoLigado = 0;
    tempoLigadoSegundos = 0;

  }
}