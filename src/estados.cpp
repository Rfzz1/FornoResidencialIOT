#include <Arduino.h>
#include "config.h"
#include "sensores.h"
#include "estados.h"

estadoSistema estadoAtual = INICIANDO;

estadoForno estadoFornoAtual = FORNO_DESLIGADO;
estadoForno estadoFornoAnterior = FORNO_DESLIGADO;

void definirEstadoSistema() {

  switch (estadoAtual) {

    case INICIANDO:

      estadoAtual = SEGURO;

      break;

    case SEGURO:

      if (TEMP_ATUAL >= TEMP_ALERTA_ENTRADA) {

        estadoAtual = ALERTA;

      }

      break;

    case ALERTA:

      if (TEMP_ATUAL >= TEMP_CRITICA_ENTRADA) {

        estadoAtual = CRITICO;

      } else if (TEMP_ATUAL < TEMP_ALERTA_SAIDA) {

        estadoAtual = SEGURO;

      }

      break;

    case CRITICO:

      if (TEMP_ATUAL < TEMP_CRITICA_SAIDA) {

        estadoAtual = ALERTA;

      }

      break;

    case ERRO_SENSOR:

      if (temperaturaValida()) {

        estadoAtual = SEGURO;

      }

      break;
  }
}

void definirEstadoForno() {

  estadoFornoAnterior = estadoFornoAtual;

  if (TEMP_ATUAL < 40) {

    estadoFornoAtual = FORNO_DESLIGADO;

  } else if (TEMP_ATUAL > ULTIMA_TEMP + MARGEM_ESTABILIDADE) {

    estadoFornoAtual = FORNO_AQUECENDO;

  } else if (TEMP_ATUAL < ULTIMA_TEMP - MARGEM_ESTABILIDADE) {

    estadoFornoAtual = FORNO_ESFRIANDO;

  } else {

    estadoFornoAtual = FORNO_ATIVO;

  }

  //Registra mudança de estado do forno
  if (estadoFornoAnterior != estadoFornoAtual) {

    Serial.print("Mudou de ");
    Serial.print(estadoFornoAnterior);

    Serial.print(" para ");

    Serial.println(estadoFornoAtual);

  }
}

void atualizarEstadoSistema() {
  if (millis() - milisEstabilizarTermopar < 2000) {

    estadoAtual = INICIANDO;
    return;

  } else if (!temperaturaValida()) {

    estadoAtual = ERRO_SENSOR;
    return;

  } else {
    definirEstadoSistema();
  }
}

void atualizarEstadoForno() {
  if (millis() - milisEstabilizarTermopar < 2000) {

    estadoFornoAtual = FORNO_DESLIGADO;
    return;

  } else if (!temperaturaValida()) {

    estadoFornoAtual = FORNO_DESLIGADO;
    return;

  } else {
    definirEstadoForno();
  }
}