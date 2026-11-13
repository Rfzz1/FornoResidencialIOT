#include <Arduino.h>
#include "config.h"
#include "sensores.h"
#include "estados.h"
#include "telemetria.h"

static unsigned long milisDuracaoEstado = 0;

static unsigned long milisUltimaJanela = 0;
static double tempReferenciaJanela = 0;
const unsigned long TEMPO_JANELA_TERMICA = 30000;

estadoForno estadoFornoAnterior;

// Só CALCULA para qual estado o sistema deve ir e devolve.
// NÃO altera condicoes.estadoAtual: quem faz isso é a taskCerebro, DEPOIS de
// comparar o estado novo com o atual e publicar os eventos de entrada/saída.
// (Antes esta função já gravava o estado novo; a comparação na taskCerebro
//  dava sempre "igual" e nenhum alerta/notificação era disparado.)
estadoSistema definirEstadoSistema()
{
  estadoSistema novo = condicoes.estadoAtual;

  switch (condicoes.estadoAtual)
  {

  case INICIANDO:

    novo = SEGURO;

    break;

  case SEGURO:

    if (condicoes.TEMP_ATUAL >= TEMP_ALERTA_ENTRADA)
    {

      novo = ALERTA;
    }

    break;

  case ALERTA:

    if (condicoes.TEMP_ATUAL >= TEMP_CRITICA_ENTRADA)
    {

      novo = CRITICO;
    }
    else if (condicoes.TEMP_ATUAL < TEMP_ALERTA_SAIDA)
    {
      novo = SEGURO;
    }

    break;

  case CRITICO:

    if (condicoes.TEMP_ATUAL < TEMP_CRITICA_SAIDA)
    {

      novo = ALERTA;
    }

    break;

  case ERRO_SENSOR:

    // Só chega aqui com o sensor já lendo de novo (a taskCerebro mantém
    // ERRO_SENSOR enquanto as leituras falham). Volta direto para o estado
    // que a temperatura atual indica, em vez de passar sempre por SEGURO.
    if (condicoes.TEMP_ATUAL >= TEMP_CRITICA_ENTRADA)
    {
      novo = CRITICO;
    }
    else if (condicoes.TEMP_ATUAL >= TEMP_ALERTA_ENTRADA)
    {
      novo = ALERTA;
    }
    else
    {
      novo = SEGURO;
    }

    break;
  }

  return novo;
}

estadoForno definirEstadoForno()
{
  estadoFornoAnterior = condicoes.estadoFornoAtual;

  // 1. Estado Desligado: avaliado de forma imediata
  if (condicoes.TEMP_ATUAL < 60)
  {
    condicoes.estadoFornoAtual = FORNO_DESLIGADO;
    condicoes.sessaoIniciada = false;
    // Reseta a referência para evitar saltos bruscos caso o forno volte a ligar
    tempReferenciaJanela = condicoes.TEMP_ATUAL;

    return condicoes.estadoFornoAtual;
  }

  // 2. Inicializa a janela de referência na primeira execução válida
  if (tempReferenciaJanela == 0)
  {
    tempReferenciaJanela = condicoes.TEMP_ATUAL;
    milisUltimaJanela = millis();
    condicoes.sessaoIniciada = true;
  }

  // 3. Avalia a tendência térmica apenas quando o tempo da janela expirar
  if (millis() - milisUltimaJanela >= TEMPO_JANELA_TERMICA)
  {

    // Compara a temperatura de agora com a de 30 segundos atrás
    if (condicoes.TEMP_ATUAL > tempReferenciaJanela + MARGEM_ESTABILIDADE)
    {
      condicoes.estadoFornoAtual = FORNO_AQUECENDO;
    }
    else if (condicoes.TEMP_ATUAL < tempReferenciaJanela - MARGEM_ESTABILIDADE)
    {
      condicoes.estadoFornoAtual = FORNO_ESFRIANDO;
    }
    else
    {
      condicoes.estadoFornoAtual = FORNO_ATIVO;
    }

    // 4. Salva a temperatura e o tempo atuais para a próxima janela
    tempReferenciaJanela = condicoes.TEMP_ATUAL;
    milisUltimaJanela = millis();

    return condicoes.estadoFornoAtual;
  }
  else
  {
    // 5. Se a janela ainda não expirou, mantém o estado anterior
    return estadoFornoAnterior;
  }
}
