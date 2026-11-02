#ifndef UTILS_H
#define UTILS_H 

#include <Arduino.h>
#include "estados.h"

enum eventoSistema {
        NENHUM,
        ALERTA_ENTRADA,
        ALERTA_SAIDA,
        CRITICO_ENTRADA,
        CRITICO_SAIDA,
        ERRO_SENSOR_ENTRADA,
        ERRO_SENSOR_SAIDA
    };

String obterEstadoSistemaTexto(estadoSistema estado);
String obterEstadoFornoTexto(estadoForno estado);
String obterEventoSistemaTexto(eventoSistema eventoSistema);

#endif