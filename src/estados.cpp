#include <Arduino.h>
#include "config.h"
#include "sensores.h"
#include "estados.h"

estadoSistema estadoAtual = INICIANDO;

estadoForno estadoFornoAtual = FORNO_DESLIGADO;
estadoForno estadoFornoAnterior = FORNO_DESLIGADO;

void definirEstadoSistema() {

  switch (estadoAtual) {

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
}