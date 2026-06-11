#ifndef LOGS_H
#define LOGS_H

#include <Arduino.h>
#include "estados.h"

void enviarDadosJSON();
String obterEstadoSistemaTexto(estadoSistema estado);
String obterEstadoFornoTexto();
void atualizarEnvioLogs();
void salvarLogEstado();

#endif