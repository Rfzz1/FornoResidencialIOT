#ifndef EVENTOS_H
#define EVENTOS_H
    #include <Arduino.h>
    #include "config.h"
    #include "estados.h"
    #include "telemetria.h"

    void processarEventos();
    bool entrouEstado(estadoSistema estado);
    bool saiuEstado(estadoSistema estado);

#endif