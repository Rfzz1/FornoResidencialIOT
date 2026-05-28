#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "logs.h"
#include "telemetria.h"
#include "iot.h"

static unsigned long milisAtualizarLogs = 0;

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

String obterEstadoFornoTexto() {

    switch (dados.estadoFornoAtual) {

        case FORNO_DESLIGADO:
            return "DESLIGADO";

        case FORNO_AQUECENDO:
            return "AQUECENDO";

        case FORNO_ATIVO:
            return "ATIVO";

        case FORNO_ESFRIANDO:
            return "ESFRIANDO";

        default:
            return "DESCONHECIDO";
    }
}

void enviarDadosJSON() {
    JsonDocument doc;

    doc["temperatura"]["atual"] = dados.TEMP_ATUAL;
    doc["temperatura"]["ultima"] = dados.ULTIMA_TEMP;
    doc["estado_sistema"] = obterEstadoSistemaTexto();
    doc["estado_forno"] = obterEstadoFornoTexto();
    doc["tempo_ligado"] = dados.tempoLigadoHoras;
    doc["horario_atual"] = obterHorarioFormatado();

    String jsonOutput;
    Serial.println("JSON Gerado: ");
    serializeJson(doc, jsonOutput);

    Serial.println(jsonOutput);
}

void atualizarEnvioLogs() {
    if(millis() - milisAtualizarLogs >= 5000) {
        milisAtualizarLogs = millis();
        enviarDadosJSON();
    }
}

void salvarLogEstado() {
    JsonDocument estadoDoc;
    Serial.printf("LOG: O sistema entrou em estado %d. Verificar imediatamente!", dados.estadoAnterior);
    estadoDoc["temperatura"]["atual"] = dados.TEMP_ATUAL;
    estadoDoc["estado_sistema"] = obterEstadoSistemaTexto();
    estadoDoc["horario_atual"] = obterHorarioFormatado();
    printLocalTime();

    String jsonEstado;
    serializeJson(estadoDoc, jsonEstado);
    Serial.printf("Detalhes do Log %d:", dados.estadoAnterior);
    Serial.println(jsonEstado);
}