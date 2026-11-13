#include "cerebroRTOS.h"
#include "config.h"
#include "sensores.h"
#include "telemetria.h"
#include "utils.h"
#include "sessao.h"
#include "ws.h"
#include "api.h"

static void publicarEvento(eventoSistema evento)
{
    if (evento == NENHUM) return;

    bool enviadoPeloWS = condicoes.wsConectado &&
        enviarEventoPeloWebSocket(obterEventoSistemaTexto(evento).c_str(), getSessaoId());

    if (!enviadoPeloWS) {
        xQueueSend(eventosQueue, &evento, 100 / portTICK_PERIOD_MS);
    }
}


void taskCerebro(void *parameter)
{

    double temperaturaAtual;
    
    static int tentativasLeitura = 0;
    static bool temLeituraValida = false;
    const int MAX_TENTATIVAS_LEITURA = 3;

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    for (;;)
    {

        //---- Temperatura ----

        xQueueReceive(temperaturaQueue, &temperaturaAtual, portMAX_DELAY);

        // Valida a leitura NOVA antes de aceitá-la. "Variação abrupta" só é
        // checada se a leitura anterior foi válida (ver temperaturaValida).
        bool compararComAnterior = temLeituraValida && tentativasLeitura == 0;

        if (temperaturaValida(temperaturaAtual, compararComAnterior))
        {
            xSemaphoreTake(mutexTelemetria, portMAX_DELAY);
                condicoes.ULTIMA_TEMP = condicoes.TEMP_ATUAL;
                condicoes.TEMP_ATUAL = temperaturaAtual;
            xSemaphoreGive(mutexTelemetria);

            Serial.printf("Temperatura atual: %.2f °C\n", condicoes.TEMP_ATUAL);
            tentativasLeitura = 0;
            temLeituraValida = true;
        }
        else
        {
            // Leitura ruim é descartada: TEMP_ATUAL mantém o último valor bom
            tentativasLeitura++;
        }

        estadoSistema novoEstadoLido = definirEstadoSistema();  // Para onde o novo estado vai ir (futuro imediato)
        estadoForno novoEstadoFornoLido = definirEstadoForno(); // Para onde o novo estado do forno vai ir (futuro imediato)

        // Erro de sensor enquanto as leituras falharem. Assim que voltar a ler
        // (tentativasLeitura zera), definirEstadoSistema() tira do ERRO_SENSOR.
        // (Antes havia "|| estadoAtual == ERRO_SENSOR", que prendia o sistema
        //  no erro para sempre.)
        if (tentativasLeitura >= MAX_TENTATIVAS_LEITURA) {
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

            publicarEvento(eventoSaida);
            publicarEvento(eventoEntrada);

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
