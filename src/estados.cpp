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

estadoSistema definirEstadoSistema()
{

  switch (condicoes.estadoAtual)
  {

  case INICIANDO:

    condicoes.estadoAtual = SEGURO;

    break;

  case SEGURO:

    if (condicoes.TEMP_ATUAL >= TEMP_ALERTA_ENTRADA)
    {

      condicoes.estadoAtual = ALERTA;
    }

    break;

  case ALERTA:

    if (condicoes.TEMP_ATUAL >= TEMP_CRITICA_ENTRADA)
    {

      condicoes.estadoAtual = CRITICO;
    }
    else if (condicoes.TEMP_ATUAL < TEMP_ALERTA_SAIDA)
    {
      condicoes.estadoAtual = SEGURO;
    }

    break;

  case CRITICO:

    if (condicoes.TEMP_ATUAL < TEMP_CRITICA_SAIDA)
    {

      condicoes.estadoAtual = ALERTA;
    }

    break;

  case ERRO_SENSOR:

    if (temperaturaValida(condicoes.TEMP_ATUAL))
    {

      condicoes.estadoAtual = SEGURO;
    }

    break;
  }

  return condicoes.estadoAtual;
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
