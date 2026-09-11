#ifndef SESSAO_H
#define SESSAO_H

void tratarFimSessao(estadoForno estadoFornoAtual, estadoForno estadoFornoAnterior);
void tratarInicioSessao(estadoForno estadoFornoAtual, estadoForno estadoFornoAnterior);
void tratarTempoSessao();
void taskSessao(void *parameter);
#endif