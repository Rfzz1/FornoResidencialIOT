#include <Arduino.h>
#include "config.h"
#include "telemetria.h"
#include "estados.h"
#include "eventos.h"
#include "logs.h" 
#include "api.h"

unsigned long ultimoTempoMudanca = 0;
const unsigned long TEMPO_DEBOUNCE = 3000;

void taskEventos(void *parameter) {

    eventoSistema evento;

    for (;;) {
    
        xQueueReceive(eventosQueue, &evento, portMAX_DELAY);
        tratarEvento(evento);

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