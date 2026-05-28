#ifndef TELEMETRIA_H
#define TELEMETRIA_H    
    #include <Arduino.h>
    #include "config.h"
    #include "estados.h"
    #include "sessao.h"

    struct Telemetria {
        bool estadoPisca = false;
        bool estadoBuzzer = false;
        bool horaSincronizada = false;

        float TEMP_ATUAL = 0;
        float ULTIMA_TEMP = 0;
        float tempoLigadoHoras = 0;
        float tempoLigadoMinutos = 0;

        unsigned long milisEstabilizarTermopar = 0;
        unsigned long tempoLigadoSegundos = 0;

        estadoSistema estadoAtual = INICIANDO;
        estadoSistema estadoAnterior = INICIANDO;

        estadoForno estadoFornoAtual = FORNO_DESLIGADO;
        estadoForno estadoFornoAnterior = FORNO_DESLIGADO;

        SessaoEstado sessaoAlerta;
        SessaoEstado sessaoCritico;
        SessaoEstado sessaoErroSensor;

        int HoraInicio = 0;
        int MinutoInicio = 0;

        int HoraFim = 0;
        int MinutoFim = 0;

        int HorarioInicio = 0;
        int HorarioFim = 0;

        int TempoAlarme = 0;
        int HoraAtual = 0;
        int MinutoAtual = 0;
        int SegundoAtual = 0;
    };

    struct SessaoEstado {

    unsigned long inicio = 0;

    unsigned long duracao = 0;

    float segundos = 0;

    float minutos = 0;

    float horas = 0;
};

extern Telemetria dados;
#endif