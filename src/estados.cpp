#include <Arduino.h>
#include "config.h"
#include "sensores.h"
#include "estados.h"
#include "telemetria.h"

static unsigned long milisDuracaoEstado = 0;

static unsigned long milisUltimaJanela = 0;
static double tempReferenciaJanela = 0;
const unsigned long TEMPO_JANELA_TERMICA = 30000;

void definirEstadoSistema() {

  switch (dados.estadoAtual) {

    case INICIANDO:

      dados.estadoAtual = SEGURO;

      break;

    case SEGURO:

      if (dados.TEMP_ATUAL >= TEMP_ALERTA_ENTRADA) {

        dados.estadoAtual = ALERTA;

      }

      if (dados.TEMP_EXT_ATUAL >= TEMP_EXT_MAXIMA) {

        dados.estadoAtual = ALERTA;

      }

      break;

    case ALERTA:

      if (dados.TEMP_ATUAL >= TEMP_CRITICA_ENTRADA) {

        dados.estadoAtual = CRITICO;

      }
      else if (dados.TEMP_ATUAL < TEMP_ALERTA_SAIDA && dados.TEMP_EXT_ATUAL < TEMP_EXT_MAXIMA) {
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

  // 1. Estado Desligado: avaliado de forma imediata
  if (dados.TEMP_ATUAL < 60) {
    dados.estadoFornoAtual = FORNO_DESLIGADO;
    dados.sessaoIniciada = false;
    // Reseta a referência para evitar saltos bruscos caso o forno volte a ligar
    tempReferenciaJanela = dados.TEMP_ATUAL; 
    return;
  }

  // 2. Inicializa a janela de referência na primeira execução válida
  if (tempReferenciaJanela == 0) {
    tempReferenciaJanela = dados.TEMP_ATUAL;
    milisUltimaJanela = millis();
    dados.sessaoIniciada = true;
  }

  // 3. Avalia a tendência térmica apenas quando o tempo da janela expirar
  if (millis() - milisUltimaJanela >= TEMPO_JANELA_TERMICA) {
    
    // Compara a temperatura de agora com a de 30 segundos atrás
    if (dados.TEMP_ATUAL > tempReferenciaJanela + MARGEM_ESTABILIDADE) {
      dados.estadoFornoAtual = FORNO_AQUECENDO;
      
    } else if (dados.TEMP_ATUAL < tempReferenciaJanela - MARGEM_ESTABILIDADE) {
      dados.estadoFornoAtual = FORNO_ESFRIANDO;
      
    } else {
      dados.estadoFornoAtual = FORNO_ATIVO;
    }

    // 4. Salva a temperatura e o tempo atuais para a próxima janela
    tempReferenciaJanela = dados.TEMP_ATUAL;
    milisUltimaJanela = millis();
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