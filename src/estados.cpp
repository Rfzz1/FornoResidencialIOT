#include <Arduino.h>
#include "config.h"
#include "sensores.h"
#include "estados.h"
#include "telemetria.h"

static unsigned long milisDuracaoEstado = 0;

static unsigned long milisUltimaJanela = 0;
static double tempReferenciaJanela = 0;
const unsigned long TEMPO_JANELA_TERMICA = 30000;
estadoSistema estadoSistemaAtual;
estadoForno estadoFornoAtual;
estadoForno estadoFornoAnterior;

estadoSistema definirEstadoSistema() {

  switch (estadoSistemaAtual) {

    case INICIANDO:

      estadoSistemaAtual = SEGURO;

      break;

    case SEGURO:

      if (dados.TEMP_ATUAL >= TEMP_ALERTA_ENTRADA) {

        estadoSistemaAtual = ALERTA;

      }

      if (dados.TEMP_EXT_ATUAL >= TEMP_EXT_MAXIMA) {

        estadoSistemaAtual = ALERTA;

      }

      break;

    case ALERTA:

      if (dados.TEMP_ATUAL >= TEMP_CRITICA_ENTRADA) {

        estadoSistemaAtual = CRITICO;

      }
      else if (dados.TEMP_ATUAL < TEMP_ALERTA_SAIDA && dados.TEMP_EXT_ATUAL < TEMP_EXT_MAXIMA) {
        estadoSistemaAtual = SEGURO;
      }

      break;

    case CRITICO:

      if (dados.TEMP_ATUAL < TEMP_CRITICA_SAIDA) {

        estadoSistemaAtual = ALERTA;

      }

      break;

    case ERRO_SENSOR:

      if (temperaturaValida()) {

        estadoSistemaAtual = SEGURO;

      }

      break;
  }

  return estadoSistemaAtual;
}

estadoForno definirEstadoForno() {
  estadoFornoAnterior = estadoFornoAtual;

  // 1. Estado Desligado: avaliado de forma imediata
  if (dados.TEMP_ATUAL < 60) {
    estadoFornoAtual = FORNO_DESLIGADO;
    dados.sessaoIniciada = false;
    // Reseta a referência para evitar saltos bruscos caso o forno volte a ligar
    tempReferenciaJanela = dados.TEMP_ATUAL;

    return estadoFornoAtual;
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
      estadoFornoAtual = FORNO_AQUECENDO;
      
    } else if (dados.TEMP_ATUAL < tempReferenciaJanela - MARGEM_ESTABILIDADE) {
      estadoFornoAtual = FORNO_ESFRIANDO;
      
    } else {
      estadoFornoAtual = FORNO_ATIVO;
    }

    // 4. Salva a temperatura e o tempo atuais para a próxima janela
    tempReferenciaJanela = dados.TEMP_ATUAL;
    milisUltimaJanela = millis();

    return estadoFornoAtual;
  } else {
    // 5. Se a janela ainda não expirou, mantém o estado anterior
    return estadoFornoAnterior;
  }
}
