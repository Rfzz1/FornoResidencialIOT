#include <Arduino.h>
#include "config.h"
#include "sensores.h"
#include "estados.h"
#include "telemetria.h"

// Histórico das últimas leituras para a janela deslizante do estado do forno.
// Uma posição a mais que a janela: assim a leitura mais antiga é de exatamente N segundos atrás.
static const uint8_t TAM_HISTORICO = JANELA_TENDENCIA_SEG + 1;
static float historicoTemperatura[TAM_HISTORICO];
static uint8_t indiceHistorico = 0;
static uint8_t quantidadeHistorico = 0;

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

// Calcula o estado do forno com uma JANELA DESLIZANTE e devolve.
// Deve ser chamada UMA vez por leitura (1 s), e só pela taskCerebro.
// Assim como definirEstadoSistema(), NÃO altera condicoes: quem grava é a taskCerebro.
//
// Antes: o estado só era recalculado a cada 30 s (comparando com 30 s antes),
// então podia mostrar AQUECENDO por até 30 s com a temperatura já caindo.
// Agora: a cada segundo compara "agora" com "N segundos atrás".
estadoForno definirEstadoForno()
{
  const float atual = condicoes.TEMP_ATUAL;
  const estadoForno estadoAtual = condicoes.estadoFornoAtual;

  // 1. Desligado: avaliado na hora, e o histórico recomeça do zero
  if (atual < TEMP_FORNO_LIGADO)
  {
    indiceHistorico = 0;
    quantidadeHistorico = 0;
    return FORNO_DESLIGADO;
  }

  // 2. Guarda a leitura no histórico circular
  historicoTemperatura[indiceHistorico] = atual;
  indiceHistorico = (indiceHistorico + 1) % TAM_HISTORICO;
  if (quantidadeHistorico < TAM_HISTORICO) quantidadeHistorico++;

  // 3. Ainda não há N segundos de histórico (acabou de passar de 60 °C).
  //    Se veio de desligado, está esquentando; senão, mantém o estado.
  if (quantidadeHistorico < TAM_HISTORICO)
  {
    return (estadoAtual == FORNO_DESLIGADO) ? FORNO_AQUECENDO : estadoAtual;
  }

  // 4. A posição que será sobrescrita a seguir guarda a leitura mais antiga (N s atrás)
  const float diferenca = atual - historicoTemperatura[indiceHistorico];

  // 5. Variação grande: entra (ou troca direto) para AQUECENDO / ESFRIANDO
  if (diferenca >= MARGEM_ENTRAR_TENDENCIA)  return FORNO_AQUECENDO;
  if (diferenca <= -MARGEM_ENTRAR_TENDENCIA) return FORNO_ESFRIANDO;

  // 6. Zona intermediária (histerese): só sai de AQUECENDO/ESFRIANDO quando a
  //    variação ficar abaixo da margem de saída. Evita o estado ficar piscando.
  if (estadoAtual == FORNO_AQUECENDO && diferenca >= MARGEM_SAIR_TENDENCIA)  return FORNO_AQUECENDO;
  if (estadoAtual == FORNO_ESFRIANDO && diferenca <= -MARGEM_SAIR_TENDENCIA) return FORNO_ESFRIANDO;

  return FORNO_ATIVO;
}
