#include "cerebroRTOS.h"
#include "config.h"
#include "sensores.h"
#include "telemetria.h"
#include "utils.h"
#include "sessao.h"
#include "buzzer.h"

void taskCerebro(void *parameter)
{

    double temperaturaAtual;
    
    static int tentativasLeitura = 0;
    const int MAX_TENTATIVAS_LEITURA = 3;
    bool sucessoLeitura = false;

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    for (;;)
    {

        //---- Temperatura ----

        xQueueReceive(temperaturaQueue, &temperaturaAtual, portMAX_DELAY);

        condicoes.ULTIMA_TEMP = condicoes.TEMP_ATUAL;

        if (temperaturaValida(temperaturaAtual))
        {
            condicoes.TEMP_ATUAL = temperaturaAtual;
            Serial.printf("Temperatura atual: %.2f °C\n", condicoes.TEMP_ATUAL);
            tentativasLeitura = 0;

            if (condicoes.estadoAtual == ERRO_SENSOR) {
                Serial.println("Sensor recuperado! Saindo do estado de erro.");
                desligarBuzzer();
        
                eventoSistema eventoRecuperacao = ERRO_SENSOR_SAIDA;
                xQueueSend(eventosQueue, &eventoRecuperacao, 0);
                condicoes.estadoAtual = definirEstadoSistema();
            }
        }
        else
        {
            tentativasLeitura++;
        }

        estadoSistema novoEstadoLido = definirEstadoSistema();  // Para onde o novo estado vai ir (futuro imediato)
        estadoForno novoEstadoFornoLido = definirEstadoForno(); // Para onde o novo estado do forno vai ir (futuro imediato)

        if (tentativasLeitura >= MAX_TENTATIVAS_LEITURA || condicoes.estadoAtual == ERRO_SENSOR) {
            novoEstadoLido = ERRO_SENSOR;
        }

        //---- Eventos ----

        if (novoEstadoLido != condicoes.estadoAtual)
        {

            eventoSistema eventoEntrada =  NENHUM;  
            eventoSistema eventoSaida = NENHUM;
            
            //saindo
            if (condicoes.estadoAtual == ALERTA) {
                eventoSaida = ALERTA_SAIDA;
            } else if (condicoes.estadoAtual == CRITICO) {
                eventoSaida = CRITICO_SAIDA;
            } else if (condicoes.estadoAtual == ERRO_SENSOR) {
                eventoSaida = ERRO_SENSOR_SAIDA;
            }

            //entrando
            if (novoEstadoLido == ALERTA) {
                eventoEntrada = ALERTA_ENTRADA; 
            } else if (novoEstadoLido == CRITICO) {
                eventoEntrada = CRITICO_ENTRADA;
            } else if (novoEstadoLido == ERRO_SENSOR) {
                eventoEntrada = ERRO_SENSOR_ENTRADA;
            } 

            if (eventoSaida != NENHUM) {
                xQueueSend(eventosQueue, &eventoSaida, 100/portTICK_PERIOD_MS);
            }

            if (eventoEntrada != NENHUM) {
                xQueueSend(eventosQueue, &eventoEntrada, 100/portTICK_PERIOD_MS);
            }

            xSemaphoreTake(mutexEstadoSistema, portMAX_DELAY);

            condicoes.estadoAtual = novoEstadoLido; // Onde o estado do sistema está nesse momento (passado recente) - último registro do estado do sistema

            xSemaphoreGive(mutexEstadoSistema);
        }

        if (novoEstadoFornoLido != condicoes.estadoFornoAtual)
        {

            xQueueSend(eventosFornoQueue, &novoEstadoFornoLido, 100 / portTICK_PERIOD_MS);

            xSemaphoreTake(mutexEstadoForno, portMAX_DELAY);

            condicoes.estadoFornoAnterior = condicoes.estadoFornoAtual;
            condicoes.estadoFornoAtual = novoEstadoFornoLido; // Onde o estado do forno está nesse momento (passado recente) - último registro

            xSemaphoreGive(mutexEstadoForno);
        }
    }
}
