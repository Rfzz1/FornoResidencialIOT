#include <Arduino.h>
#include "config.h"
#include "telemetria.h"
#include "estados.h"
#include "eventos.h"
#include "logs.h"
#include "api.h"

unsigned long ultimoTempoMudanca = 0;
const unsigned long TEMPO_DEBOUNCE = 3000;

static QueueHandle_t filaEventos = NULL;

static void taskProcessarEventos(void *pvParameters) {
    eventoSistema evento;
    while (1) {
        // Dorme até chegar um evento na fila
        if (xQueueReceive(filaEventos, &evento, portMAX_DELAY) == pdPASS) {
            tratarEvento(evento);
        }
    }
}
void inicializarEventos() {
    // 1. Cria a fila interna
    filaEventos = xQueueCreate(TAMANHO_FILA_EVENTOS, sizeof(eventoSistema));

    // 2. Dispara a task silenciosamente em segundo plano
    if (filaEventos != NULL) {
        xTaskCreate(
            taskProcessarEventos,
            "TaskEventos",
            4096,
            NULL,
            1,
            NULL
        );
    }
}

void adicionarEvento(eventoSistema evento) {
    if (filaEventos != NULL) {
        // Envia para a fila do FreeRTOS sem bloquear a CPU
        if (xQueueSend(filaEventos, &evento, (TickType_t)0) != pdPASS) {
            Serial.println("ERRO: fila de eventos cheia");
        }
    }
}

void tratarEvento(eventoSistema evento) {
    switch (evento) {
        case ALERTA_ENTRADA:

            iniciarSessaoEstado(
                dados.sessaoAlerta
            );
            enviarEvento("ALERTA_ENTRADA");

            salvarLogEstado();

            Serial.println("Entrou em ALERTA");

            break;
        case ALERTA_SAIDA:

            finalizarSessaoEstado(
                dados.sessaoAlerta
            );
            enviarEvento("ALERTA_SAIDA");

            exibirDuracaoEstado(
                ALERTA,
                dados.sessaoAlerta
            );

            break;
        case CRITICO_ENTRADA:
            
            iniciarSessaoEstado(
                dados.sessaoCritico
            );
            enviarEvento("CRITICO_ENTRADA");

            salvarLogEstado();

            Serial.println("Entrou em CRÍTICO");
            break;
        case CRITICO_SAIDA:
            finalizarSessaoEstado(
                dados.sessaoCritico
            );
            enviarEvento("CRITICO_SAIDA");

            exibirDuracaoEstado(
                CRITICO,
                dados.sessaoCritico
            );
            break;
        case ERRO_SENSOR_ENTRADA:
            iniciarSessaoEstado(
                dados.sessaoErroSensor
            );
            enviarEvento("ERRO_SENSOR_ENTRADA");

            Serial.println("Evento: Entrada no estado de Erro de Sensor");
            break;
        case ERRO_SENSOR_SAIDA:
            finalizarSessaoEstado(
                dados.sessaoErroSensor
            );
            enviarEvento("ERRO_SENSOR_SAIDA");
            
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

void atualizarEstadoLogico(estadoSistema novoEstadoLido) {
    if (novoEstadoLido != dados.estadoAtual) {
        if (millis() - ultimoTempoMudanca > TEMPO_DEBOUNCE) {
            dados.estadoAnterior = dados.estadoAtual;
            dados.estadoAtual = novoEstadoLido;
            ultimoTempoMudanca = millis();
        }
    } else {
        // Reseta o tempo se a leitura voltou ao normal antes de confirmar
        ultimoTempoMudanca = millis();
    }
}