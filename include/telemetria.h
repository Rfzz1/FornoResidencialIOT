#ifndef TELEMETRIA_H
#define TELEMETRIA_H    
    #include <Arduino.h>
    #include "config.h"
    #include "estados.h"

    struct SessaoEstado {

        unsigned long inicio = 0;

        unsigned long duracao = 0;

        uint32_t segundos = 0;

        uint32_t minutos = 0;

        uint32_t horas = 0;
    };

    struct Telemetria {
        bool estadoPisca = false;
        bool estadoBuzzerPisca = false;
        bool buzzerAtivaHorario = false;
        bool horaSincronizada = false;
        bool timeoutNTP = false;
        bool espConfigurado = false;
        bool buzzerMutado = false;
        bool sessaoIniciada = false;

        float TEMP_ATUAL = 0;
        float ULTIMA_TEMP = 0;
        float TEMP_EXT_ATUAL = 0;
        uint32_t tempoLigadoHoras = 0;
        uint32_t tempoLigadoMinutos = 0;

        unsigned long milisEstabilizarTermopar = 0;
        unsigned long tempoLigadoSegundos = 0;
        unsigned long inicioTentativa = 0;

        estadoSistema estadoAtual = INICIANDO;
        estadoSistema estadoAnterior = INICIANDO;

        estadoForno estadoFornoAtual = FORNO_DESLIGADO;
        estadoForno estadoFornoAnterior = FORNO_DESLIGADO;

        SessaoEstado sessaoAlerta;
        SessaoEstado sessaoCritico;
        SessaoEstado sessaoErroSensor;

        uint32_t HoraInicio = 0;
        uint32_t MinutoInicio = 0;

        uint32_t HoraFim = 0;
        uint32_t MinutoFim = 0;

        uint32_t HorarioInicio = 0;
        uint32_t HorarioFim = 0;

        uint32_t TempoAlarme = 0;
        uint32_t HoraAtual = 0;
        uint32_t MinutoAtual = 0;
        uint32_t SegundoAtual = 0;
    };

extern Telemetria dados;
#endif