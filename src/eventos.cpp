#include <Arduino.h>
#include "config.h"
#include "telemetria.h"
#include "estados.h"
#include "eventos.h"
#include "logs.h"

eventoSistema filaEventos[TAMANHO_FILA_EVENTOS];
int inicioFila = 0;
int fimFila = 0;

void adicionarEvento(eventoSistema evento) {

    int proximo =
        (fimFila + 1) % TAMANHO_FILA_EVENTOS;

    if (proximo == inicioFila) {

        Serial.println(
            "ERRO: fila cheia"
        );

        return;
    }

    filaEventos[fimFila] = evento;

    fimFila = proximo;
}

void processarFilaEventos() {
    while (inicioFila != fimFila) {
        eventoSistema evento = filaEventos[inicioFila];
        inicioFila = (inicioFila + 1) % TAMANHO_FILA_EVENTOS;

        tratarEvento(evento);
    }
}

void tratarEvento(eventoSistema evento) {
    switch (evento) {
        case ALERTA_ENTRADA:

            iniciarSessaoEstado(
                dados.sessaoAlerta
            );

            salvarLogEstado();

            Serial.println("Entrou em ALERTA");

            break;
        case ALERTA_SAIDA:

            finalizarSessaoEstado(
                dados.sessaoAlerta
            );

            exibirDuracaoEstado(
                ALERTA,
                dados.sessaoAlerta
            );

            break;
        case CRITICO_ENTRADA:
            
            iniciarSessaoEstado(
                dados.sessaoCritico
            );
            salvarLogEstado();
            Serial.println("Entrou em CRÍTICO");
            break;
        case CRITICO_SAIDA:
            finalizarSessaoEstado(
                dados.sessaoCritico
            );
            exibirDuracaoEstado(
                CRITICO,
                dados.sessaoCritico
            );
            break;
        case ERRO_SENSOR_ENTRADA:
            iniciarSessaoEstado(
                dados.sessaoErroSensor
            );
            Serial.println("Evento: Entrada no estado de Erro de Sensor");
            break;
        case ERRO_SENSOR_SAIDA:
            finalizarSessaoEstado(
                dados.sessaoErroSensor
            );
            exibirDuracaoEstado(
                ERRO_SENSOR,
                dados.sessaoErroSensor
            );
            Serial.println("Evento: Saída do estado de Erro de Sensor");
            break;
        default:
            break;
    }
}

void processarEventos() {

    if (entrouEstado(ALERTA)) {
        adicionarEvento(ALERTA_ENTRADA);
    }

    if (saiuEstado(ALERTA)) {
        adicionarEvento(ALERTA_SAIDA);
    }

    if (entrouEstado(CRITICO)) {
        adicionarEvento(CRITICO_ENTRADA);
    }

    if (saiuEstado(CRITICO)) {
        adicionarEvento(CRITICO_SAIDA);
    }

    if (entrouEstado(ERRO_SENSOR)) {
        adicionarEvento(ERRO_SENSOR_ENTRADA);
    }

    if (saiuEstado(ERRO_SENSOR)) {
        adicionarEvento(ERRO_SENSOR_SAIDA);
    }

    dados.estadoAnterior = dados.estadoAtual;
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

void exibirDuracaoEstado(estadoSistema estado, const SessaoEstado &sessao) {

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