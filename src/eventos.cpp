#include <Arduino.h>
#include "config.h"
#include "telemetria.h"
#include "estados.h"
#include "eventos.h"
#include "logs.h"

static unsigned long milisDuracaoEstado = 0;

void processarEventos() {

    if (entrouEstado(ALERTA)) {

        Serial.println("Entrou em ALERTA");

    } 

    if (entrouEstado(CRITICO)) {

        Serial.println("Entrou em CRITICO");

        milisDuracaoEstado = millis();

        salvarLogCritico();
        
    } else if (saiuEstado(CRITICO)) {

        unsigned long duracao = millis() - milisDuracaoEstado;

        Serial.printf(
            "Sessão crítica durou %lu segundos\n",
            duracao / 1000
        );
    }

    if (entrouEstado(ERRO_SENSOR)) {

        Serial.println("Sensor falhou");
    }
}

bool entrouEstado(estadoSistema estado) {

    return (
        dados.estadoAtual == estado &&
        dados.estadoAnterior != estado
    );
}

bool saiuEstado(estadoSistema estado) {

    return (
        dados.estadoAnterior == estado &&
        dados.estadoAtual != estado
    );
}