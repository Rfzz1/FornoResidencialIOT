#ifndef TELEMETRIA_H
#define TELEMETRIA_H    
    #include <Arduino.h>
    #include "config.h"
    #include "estados.h"


    struct Telemetria {
        bool estadoPisca = false;
        bool estadoBuzzerPisca = false;
        bool buzzerAtivaHorario = false;
        bool buzzerTemporizadorPisca = false;
        bool horaSincronizada = false;
        bool timeoutNTP = false;
        bool espConfigurado = false;
        bool buzzerMutado = false;
        bool sessaoIniciada = false;
        bool fezLogin = false;
        bool temporizadorLigado = false;

        float TEMP_ATUAL = 0;
        float ULTIMA_TEMP = 0;
        float TEMP_EXT_ATUAL = 0;
        uint32_t tempoLigadoHoras = 0;
        uint32_t tempoLigadoMinutos = 0;
        
        uint32_t duracaoSegundosTemporizador = 0;

        unsigned long milisEstabilizarTermopar = 0;
        unsigned long tempoLigadoSegundos = 0;
        unsigned long inicioTentativa = 0;
        unsigned long mensagemChegou = 0;

        estadoSistema estadoAtual = INICIANDO;
        estadoSistema estadoAnterior = INICIANDO;

        estadoForno estadoFornoAtual = FORNO_DESLIGADO;
        estadoForno estadoFornoAnterior = FORNO_DESLIGADO;

        uint32_t HoraFim = 0;
        uint32_t MinutoFim = 0;

        uint32_t HoraAtual = 0;
        uint32_t MinutoAtual = 0;
        uint32_t SegundoAtual = 0;

        String serialNumber;
        String deviceSecret;
        String tokenUsuario;
    };

extern Telemetria dados;
#endif