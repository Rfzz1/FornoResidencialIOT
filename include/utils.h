#ifndef UTILS_H
#define UTILS_H 

#include <Arduino.h>

String obterEstadoSistemaTexto(estadoSistema estado);
String obterEstadoFornoTexto(estadoForno estado);
String obterEventoSistemaTexto(eventoSistema eventoSistema);

enum eventoSistema {
        NENHUM,
        ALERTA_ENTRADA,
        ALERTA_SAIDA,
        CRITICO_ENTRADA,
        CRITICO_SAIDA,
        ERRO_SENSOR_ENTRADA,
        ERRO_SENSOR_SAIDA
    };

    enum eventoForno {
        FORNO_DESLIGADO_ENTRADA,
        FORNO_DESLIGADO_SAIDA,
        FORNO_AQUECENDO_ENTRADA,
        FORNO_AQUECENDO_SAIDA,
        FORNO_ATIVO_ENTRADA,
        FORNO_ATIVO_SAIDA,
        FORNO_ESFRIANDO_ENTRADA,
        FORNO_ESFRIANDO_SAIDA
    };

#endif