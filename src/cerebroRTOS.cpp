#include "cerebroRTOS.h"
#include "config.h"
#include "sensores.h"
#include "telemetria.h"
#include "utils.h"
#include "sessao.h"

void taskCerebro(void *parameter)
{

    double temperaturaAtual;

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    for (;;)
    {

        //---- Temperatura ----

        xQueueReceive(temperaturaQueue, &temperaturaAtual, portMAX_DELAY);

        condicoes.ULTIMA_TEMP = condicoes.TEMP_ATUAL;

        condicoes.TEMP_ATUAL = temperaturaAtual;

        if (temperaturaValida())
        {

            Serial.printf("Temperatura atual: %.2f °C\n", condicoes.TEMP_ATUAL);
        }
        else
        {

            Serial.println("Leitura de temperatura inválida.");

            continue; // Skip further processing if the temperature is invalid
        }

        estadoSistema novoEstadoLido = definirEstadoSistema(); // Para onde o novo estado vai ir (futuro imediato)

        estadoForno novoEstadoFornoLido = definirEstadoForno(); // Para onde o novo estado do forno vai ir (futuro imediato)

        //---- Eventos ----

        if (novoEstadoLido != condicoes.estadoAtual)
        {

            eventoSistema evento;

            switch (novoEstadoLido)
            {

            case SEGURO:

                // Se o novo estado lido estiver segureo e o estado atual for lido como 'ALERTA', quer dizer que entrou em alerta e assim segue para os outros estados

                if (condicoes.estadoAtual == ALERTA)
                {

                    evento = ALERTA_SAIDA;
                }
                else if (condicoes.estadoAtual == CRITICO)
                {

                    evento = CRITICO_SAIDA;
                }
                else if (condicoes.estadoAtual == ERRO_SENSOR)
                {

                    evento = ERRO_SENSOR_SAIDA;
                }

                break;

            case ALERTA:

                if (condicoes.estadoAtual == SEGURO)
                {

                    evento = ALERTA_ENTRADA;
                }
                else if (condicoes.estadoAtual == CRITICO)
                {

                    evento = CRITICO_SAIDA;
                }
                else if (condicoes.estadoAtual == ERRO_SENSOR)
                {

                    evento = ERRO_SENSOR_SAIDA;
                }

                break;

            case CRITICO:

                if (condicoes.estadoAtual == SEGURO)
                {

                    evento = CRITICO_ENTRADA;
                }
                else if (condicoes.estadoAtual == ALERTA)
                {

                    evento = ALERTA_SAIDA;
                }
                else if (condicoes.estadoAtual == ERRO_SENSOR)
                {

                    evento = ERRO_SENSOR_SAIDA;
                }

                break;

            case ERRO_SENSOR:

                if (condicoes.estadoAtual == SEGURO)
                {

                    evento = ERRO_SENSOR_ENTRADA;
                }
                else if (condicoes.estadoAtual == ALERTA)
                {

                    evento = ALERTA_SAIDA;
                }
                else if (condicoes.estadoAtual == CRITICO)
                {

                    evento = CRITICO_SAIDA;
                }

                break;
            }

            xQueueSend(eventosQueue, &evento, 0);

            xSemaphoreTake(mutexEstadoSistema, portMAX_DELAY);

            condicoes.estadoAtual = novoEstadoLido; // Onde o estado do sistema está nesse momento (passado recente) - último registro do estado do sistema

            xSemaphoreGive(mutexEstadoSistema);
        }

        if (novoEstadoFornoLido != condicoes.estadoFornoAtual)
        {

            //---- Estado ----

            xQueueSend(eventosFornoQueue, &novoEstadoFornoLido, 0);

            xSemaphoreTake(mutexEstadoForno, portMAX_DELAY);

            condicoes.estadoFornoAnterior = condicoes.estadoFornoAtual;
            condicoes.estadoFornoAtual = novoEstadoFornoLido; // Onde o estado do forno está nesse momento (passado recente) - último registro

            xSemaphoreGive(mutexEstadoForno);
        }
    }
}
