#ifndef EVENTOS_H
#define EVENTOS_H
    #include <Arduino.h>
    #include "config.h"
    #include "estados.h"
    #include "telemetria.h"

    enum eventoSistema {
        NENHUM,
        ALERTA_ENTRADA,
        ALERTA_SAIDA,
        CRITICO_ENTRADA,
        CRITICO_SAIDA,
        ERRO_SENSOR_ENTRADA,
        ERRO_SENSOR_SAIDA
    };

    bool entrouEstado(estadoSistema estado);
    bool saiuEstado(estadoSistema estado);
    void exibirDuracaoEstado(estadoSistema estado,const SessaoEstado &sessao);
    void iniciarSessaoEstado(SessaoEstado &sessao);
    void finalizarSessaoEstado(SessaoEstado &sessao);
    void adicionarEvento(eventoSistema evento);
    void processarFilaEventos();
    void processarEventos();
    void tratarEvento(eventoSistema evento);

#endif