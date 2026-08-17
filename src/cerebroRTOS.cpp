#include "cerebroRTOS.h"
#include "config.h"
#include "sensores.h"
#include "telemetria.h"

void taskCerebro(void *parameter) {

    float temperaturaAtual = 0.0;
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    for (;;) {

    xQueueReceive(temperaturaQueue, &temperaturaAtual, portMAX_DELAY);

    dados.ULTIMA_TEMP = dados.TEMP_ATUAL;
    dados.TEMP_ATUAL = temperaturaAtual;

    if (temperaturaValida()) {
        Serial.printf("Temperatura atual: %.2f °C\n", dados.TEMP_ATUAL);
    } else {
        Serial.println("Leitura de temperatura inválida.");
        continue; // Skip further processing if the temperature is invalid
    }

    xSemaphoreTake(mutexEstadoForno, portMAX_DELAY);

    dados.estadoFornoAnterior = dados.estadoFornoAtual;
    dados.estadoFornoAtual = definirEstadoForno();

    xSemaphoreGive(mutexEstadoForno);

    xSemaphoreTake(mutexEstadoSistema, portMAX_DELAY);

    dados.estadoAtual = definirEstadoSistema();

    xSemaphoreGive(mutexEstadoSistema);

    }
}