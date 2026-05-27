#ifndef ESTADOS_H
#define ESTADOS_H

enum estadoSistema {
    INICIANDO,
    SEGURO,
    ALERTA,
    CRITICO,
    ERRO_SENSOR
};

enum estadoForno {
FORNO_DESLIGADO,
FORNO_AQUECENDO,
FORNO_ATIVO,
FORNO_ESFRIANDO
};

void definirEstadoSistema();
void atualizarEstadoSistema();
void definirEstadoForno();
void atualizarEstadoForno();
#endif