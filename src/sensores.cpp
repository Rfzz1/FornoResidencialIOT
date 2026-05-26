#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "sensores.h"

MAX6675 TERMOPAR(TERMOPAR_SCK, TERMOPAR_CS, TERMOPAR_SO);

float TEMP_ATUAL = 0;
float ULTIMA_TEMP = 0;

unsigned long milisLerTemp = 0;

void lerTemperatura() {

    ULTIMA_TEMP = TEMP_ATUAL;

    TEMP_ATUAL = TERMOPAR.readCelsius();

    Serial.printf(
        "Temperatura: %.2f °C\n",
        TEMP_ATUAL
    );
}

void atualizarSensores() {

  if (millis() - ultimaLeituraSensor >= 1000) {

    ultimaLeituraSensor = millis();

    lerTemperatura();

  }

}

bool temperaturaValida() {

  if (isnan(TEMP_ATUAL)) {

    Serial.println("Erro: Falha ao ler sensor!");
    return false;

  }

  if (TEMP_ATUAL < 0) {

    Serial.println("Erro: Temperatura negativa!");
    return false;

  }

  if (TEMP_ATUAL > TEMP_MAXIMA_VALIDA) {

    Serial.println("Erro: Temperatura muito alta!");
    return false;

  }

  if (fabs(TEMP_ATUAL - ULTIMA_TEMP) > 100) {

    Serial.println("Erro: Variacao abrupta!");
    return false;

  }

  return true;
}