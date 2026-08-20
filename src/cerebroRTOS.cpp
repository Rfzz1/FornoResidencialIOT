#include "cerebroRTOS.h"
#include "config.h"
#include "sensores.h"
#include "telemetria.h"
#include "utils.h"

void taskCerebro(void *parameter) {

    double temperaturaAtual;
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    for (;;) {

    //---- Temperatura ----

        xQueueReceive(temperaturaQueue, &temperaturaAtual, portMAX_DELAY);

        dados.ULTIMA_TEMP = dados.TEMP_ATUAL;
        dados.TEMP_ATUAL = temperaturaAtual;

        if (temperaturaValida()) {
            Serial.printf("Temperatura atual: %.2f °C\n", dados.TEMP_ATUAL);
        } else {
            Serial.println("Leitura de temperatura inválida.");
            continue; // Skip further processing if the temperature is invalid
        }

        //---- Estado ----

        xSemaphoreTake(mutexEstadoForno, portMAX_DELAY);

        dados.estadoFornoAnterior = dados.estadoFornoAtual;
        dados.estadoFornoAtual = definirEstadoForno();

        xSemaphoreGive(mutexEstadoForno);

        estadoSistema novoEstadoLido = definirEstadoSistema(); //Para onde o novo estado vai ir (futuro imediato)
        estadoForno novoEstadoFornoLido = definirEstadoForno(); //Para onde o novo estado do forno vai ir (futuro imediato)

        //---- Eventos ----

        if (novoEstadoLido != dados.estadoAtual) {
            
            eventoSistema evento;

            switch (novoEstadoLido) {
                case SEGURO:

                    //Se o novo estado lido estiver segureo e o estado atual for lido como 'ALERTA', quer dizer que entrou em alerta e assim segue para os outros estados
                    if (dados.estadoAtual == ALERTA) {
                        evento = ALERTA_SAIDA;
                    } else if (dados.estadoAtual == CRITICO) {
                        evento = CRITICO_SAIDA;
                    } else if (dados.estadoAtual == ERRO_SENSOR) {
                        evento = ERRO_SENSOR_SAIDA;
                    }
                    break;
                case ALERTA:
                    if (dados.estadoAtual == SEGURO) {
                        evento = ALERTA_ENTRADA;
                    } else if (dados.estadoAtual == CRITICO) {
                        evento = CRITICO_SAIDA;
                    } else if (dados.estadoAtual == ERRO_SENSOR) {
                        evento = ERRO_SENSOR_SAIDA;
                    }
                    break;
                case CRITICO:
                    if (dados.estadoAtual == SEGURO) {
                        evento = CRITICO_ENTRADA;
                    } else if (dados.estadoAtual == ALERTA) {
                        evento = ALERTA_SAIDA;
                    } else if (dados.estadoAtual == ERRO_SENSOR) {
                        evento = ERRO_SENSOR_SAIDA;
                    }
                    break;
                case ERRO_SENSOR:
                    if (dados.estadoAtual == SEGURO) {
                        evento = ERRO_SENSOR_ENTRADA;
                    } else if (dados.estadoAtual == ALERTA) {
                        evento = ALERTA_SAIDA;
                    } else if (dados.estadoAtual == CRITICO) {
                        evento = CRITICO_SAIDA;
                    }
                    break;
            }

            xQueueSend(eventosQueue, &evento, 0);
            
            xSemaphoreTake(mutexEstadoSistema, portMAX_DELAY);

            dados.estadoAtual = novoEstadoLido; //Onde o estado do sistema está nesse momento (passado recente) - último registro do estado do sistema

            xSemaphoreGive(mutexEstadoSistema);

        }

        if (novoEstadoFornoLido != dados.estadoFornoAtual) {
            
            estadoForno eventoForno;

            switch (novoEstadoFornoLido) {
                case FORNO_DESLIGADO:
                    eventoForno = FORNO_DESLIGADO;
                    break;
                case FORNO_AQUECENDO:
                    eventoForno = FORNO_AQUECENDO;
                    break;
                case FORNO_ATIVO:
                    eventoForno = FORNO_ATIVO;
                    break;
                case FORNO_ESFRIANDO:
                    eventoForno = FORNO_ESFRIANDO;
                    break;
            }

            xQueueSend(eventosFornoQueue, &eventoForno, 0);

            xSemaphoreTake(mutexEstadoForno, portMAX_DELAY);

            dados.estadoFornoAtual = novoEstadoFornoLido; //Onde o estado do forno está nesse momento (passado recente) - último registro

            xSemaphoreGive(mutexEstadoForno);

    }
}
}