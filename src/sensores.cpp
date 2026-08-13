#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "sensores.h"
#include "telemetria.h"
#include "api.h"

static unsigned long ultimaLeituraSensor = 0;
static unsigned long ultimoEnvioTemperatura = 0;
static float leituraBruta = 0;

const int TAM_MEDIA = 10;

float bufferTemperaturas[TAM_MEDIA];

int indiceBuffer = 0;

bool bufferCheio = false;

MAX6675 TERMOPAR(TERMOPAR_SCK, TERMOPAR_CS, TERMOPAR_SO);

void taskTemperatura(void *parameter) {

  for (;;) {

    float temperaturaAtual = lerTemperatura();
    xQueueSend(temperaturaQueue, &temperaturaAtual, 0);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

  }

}

float lerTemperatura() {

    leituraBruta = TERMOPAR.readCelsius();

    if (isnan(leituraBruta) || leituraBruta <= 1.0 || leituraBruta > 1000.0) {
        Serial.println("Interferência detectada! Leitura ignorada.");
        return NAN; 
    }

    bufferTemperaturas[indiceBuffer] = leituraBruta;

    indiceBuffer++;

    if (indiceBuffer >= TAM_MEDIA) {

        indiceBuffer = 0;
        bufferCheio = true;

    }

    int quantidade =
        bufferCheio ?
        TAM_MEDIA :
        indiceBuffer;

    float soma = 0;

    for (int i = 0; i < quantidade; i++) {

        soma += bufferTemperaturas[i];

    }

    dados.ULTIMA_TEMP = dados.TEMP_ATUAL;
    float temperaturaAtual = soma / quantidade;

    if (dados.sessaoIniciada) {

      Serial.printf("[REGISTRO] Temperatura: %.2f °C\n", temperaturaAtual);
    } else {
      Serial.printf("Aguardando inicio da sessão... Temp atual: %.2f°C\n", temperaturaAtual);
    }
    return temperaturaAtual;
}

void lerTemperaturaExterna() {

    bool quente = digitalRead(TERMOSTATO);

    if (quente) {

        dados.TEMP_EXT_ATUAL = TEMP_EXT_MAXIMA;

    } else {

        dados.TEMP_EXT_ATUAL = 0;

    }
}

void atualizarSensores() {

    lerTemperatura();
    lerTemperaturaExterna();
}

bool temperaturaValida() {

  if (isnan(dados.TEMP_ATUAL)) {

    Serial.println("Erro: Falha ao ler sensor!");
    return false;

  }

  if (dados.TEMP_ATUAL < 0) {

    Serial.println("Erro: Temperatura negativa!");
    return false;

  }

  if (dados.TEMP_ATUAL > TEMP_MAXIMA_VALIDA) {

    Serial.println("Erro: Temperatura muito alta!");
    return false;

  }

  if (fabs(dados.TEMP_ATUAL - dados.ULTIMA_TEMP) > 100) {

    Serial.println("Erro: Variacao abrupta!");
    return false;

  }

  return true;
}