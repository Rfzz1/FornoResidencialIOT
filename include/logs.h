#ifndef LOGS_H
#define LOGS_H

void enviarDadosJSON();
String obterEstadoSistemaTexto(estadoSistema estado);
String obterEstadoFornoTexto();
void atualizarEnvioLogs();
void salvarLogEstado();

#endif