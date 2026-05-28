#include <Arduino.h>
#include "config.h"
#include "sensores.h"
#include "estados.h"
#include "telemetria.h"

static unsigned long milisDuracaoEstado = 0;

void definirEstadoSistema() {

  switch (dados.estadoAtual) {

    case INICIANDO:

      dados.estadoAtual = SEGURO;

      break;

    case SEGURO:

      if (dados.TEMP_ATUAL >= TEMP_ALERTA_ENTRADA) {

        dados.estadoAtual = ALERTA;

      }

      break;

    case ALERTA:

      if (dados.TEMP_ATUAL >= TEMP_CRITICA_ENTRADA) {

        dados.estadoAtual = CRITICO;

      } else if (dados.TEMP_ATUAL < TEMP_ALERTA_SAIDA) {

        dados.estadoAtual = SEGURO;

      }

      break;

    case CRITICO:

      if (dados.TEMP_ATUAL < TEMP_CRITICA_SAIDA) {

        dados.estadoAtual = ALERTA;

      }

      break;

    case ERRO_SENSOR:

      if (temperaturaValida()) {

        dados.estadoAtual = SEGURO;

      }

      break;
  }
}

void definirEstadoForno() {

  dados.estadoFornoAnterior = dados.estadoFornoAtual;

  if (dados.TEMP_ATUAL < 40) {

    dados.estadoFornoAtual = FORNO_DESLIGADO;

  } else if (dados.TEMP_ATUAL > dados.ULTIMA_TEMP + MARGEM_ESTABILIDADE) {

    dados.estadoFornoAtual = FORNO_AQUECENDO;

  } else if (dados.TEMP_ATUAL < dados.ULTIMA_TEMP - MARGEM_ESTABILIDADE) {

    dados.estadoFornoAtual = FORNO_ESFRIANDO;

  } else {

    dados.estadoFornoAtual = FORNO_ATIVO;

  }
}

void atualizarEstadoSistema() {

  if (millis() - dados.milisEstabilizarTermopar < 2000) {

    dados.estadoAtual = INICIANDO;
    return;

  } else if (!temperaturaValida()) {

    dados.estadoAtual = ERRO_SENSOR;
    return;

  } else {
    definirEstadoSistema();
  }
}

void atualizarEstadoForno() {
  if (millis() - dados.milisEstabilizarTermopar < 2000) {

    dados.estadoFornoAtual = FORNO_DESLIGADO;
    return;

  } else if (!temperaturaValida()) {

    dados.estadoFornoAtual = FORNO_DESLIGADO;
    return;

  } else {
    definirEstadoForno();
  }
}