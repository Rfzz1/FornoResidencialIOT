#include "utils.h"
#include "telemetria.h"
#include "estados.h"
#include "api.h"
#include "config.h"


// =====================================================
// CONVERTE ESTADO SISTEMA PARA TEXTO
// =====================================================

String obterEstadoSistemaTexto(estadoSistema estado) {

    switch (estado) {

        case INICIANDO:
            return "INICIANDO";

        case SEGURO:
            return "SEGURO";

        case ALERTA:
            return "ALERTA";

        case CRITICO:
            return "CRITICO";

        case ERRO_SENSOR:
            return "ERRO_SENSOR";

        default:
            return "DESCONHECIDO";
    }
}

// =====================================================
// CONVERTE ESTADO FORNO PARA TEXTO
// =====================================================

String obterEstadoFornoTexto(estadoForno estadoForno) {

    switch (estadoForno) {

        case FORNO_DESLIGADO:
            return "FORNO_DESLIGADO";

        case FORNO_AQUECENDO:
            return "FORNO_AQUECENDO";

        case FORNO_ATIVO:
            return "FORNO_ATIVO";

        case FORNO_ESFRIANDO:
            return "FORNO_ESFRIANDO";

        default:
            return "DESCONHECIDO";
    }
}

// ------- CONVERTE EVENTO DO SISTEMA PARA TEXTO -------

String obterEventoSistemaTexto(eventoSistema eventoSistema) {

    switch (eventoSistema) {

        case ALERTA_ENTRADA:
            return "ALERTA_ENTRADA";

        case ALERTA_SAIDA:
            return "ALERTA_SAIDA";

        case CRITICO_ENTRADA:
            return "CRITICO_ENTRADA";

        case CRITICO_SAIDA:
            return "CRITICO_SAIDA";

        case ERRO_SENSOR_ENTRADA:
            return "ERRO_SENSOR_ENTRADA";

        case ERRO_SENSOR_SAIDA:
            return "ERRO_SENSOR_SAIDA";

    }

}