#include "cerebroRTOS.h"
#include "config.h"
#include "sensores.h"
#include "telemetria.h"

void taskCerebro(void *parameter) {

    float temperaturaAtual = 0.0;
    for (;;) {

    xQueueReceive(temperaturaQueue, &temperaturaAtual, portMAX_DELAY);

    dados.ULTIMA_TEMP = dados.TEMP_ATUAL;
    dados.TEMP_ATUAL = temperaturaAtual;

    if (temperaturaValida(true)) {
        Serial.printf("Temperatura atual: %.2f °C\n", dados.TEMP_ATUAL);
    } else {
        Serial.println("Leitura de temperatura inválida.");
        continue; // Skip further processing if the temperature is invalid
    }

    }
}