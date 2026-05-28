#include <Arduino.h>
#include "config.h"
#include "telemetria.h"
#include "estados.h"
#include "eventos.h"
#include "logs.h"

void processarEventos() {

    if (entrouEstado(ALERTA)) {

        Serial.println("Entrou em ALERTA");
        iniciarSessaoEstado(dados.sessaoAlerta);
        salvarLogEstado();

    } else if (saiuEstado(ALERTA)) {
        finalizarSessaoEstado(
            dados.sessaoAlerta
        );

        exibirDuracaoEstado(
            ALERTA,
            dados.sessaoAlerta
        );
    }

    if (entrouEstado(CRITICO)) {

        Serial.println("Entrou em CRITICO");
        iniciarSessaoEstado(dados.sessaoCritico);
        salvarLogEstado();
        
    } else if (saiuEstado(CRITICO)) {

        finalizarSessaoEstado(dados.sessaoCritico);
        exibirDuracaoEstado(
            CRITICO,
            dados.sessaoCritico
        );
    }

    if (entrouEstado(ERRO_SENSOR)) {

        Serial.println("Sensor falhou");
        iniciarSessaoEstado(dados.sessaoErroSensor);
        salvarLogEstado();
    } else if (saiuEstado(ERRO_SENSOR)) {

        finalizarSessaoEstado(dados.sessaoErroSensor);
        exibirDuracaoEstado(
            ERRO_SENSOR,
            dados.sessaoErroSensor
        );
    }
}

void iniciarSessaoEstado(SessaoEstado &sessao) {
    sessao.inicio = millis();
}

void finalizarSessaoEstado(
    SessaoEstado &sessao
) {
    sessao.duracao = millis() - sessao.inicio;

    sessao.segundos = sessao.duracao / 1000.0;

    sessao.minutos = sessao.segundos / 60.0;

    sessao.horas = sessao.minutos / 60.0;
}

void exibirDuracaoEstado(estadoSistema estado, SessaoEstado sessao) {

    Serial.printf(
        "Estado %s durou %.2f segundos "
        "(%.2f minutos, %.2f horas)\n",

        obterEstadoSistemaTexto(estado).c_str(),

        sessao.segundos,

        sessao.minutos,

        sessao.horas
    );
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