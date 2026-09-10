#ifndef SESSAO_H
#define SESSAO_H

void tratarTempoSessao();
void tratarInicioSessao(estadoForno estadoFornoAtual, estadoForno estadoFornoAnterior);
void tratarFimSessao(estadoForno estadoFornoAtual, estadoForno estadoFornoAnterior);
void taskSessao(void *parameter);
#endif