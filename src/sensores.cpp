#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "sensores.h"
#include "telemetria.h"

static unsigned long ultimaLeituraSensor = 0;

MAX6675 TERMOPAR(TERMOPAR_SCK, TERMOPAR_CS, TERMOPAR_SO);

void lerTemperatura() {

    dados.ULTIMA_TEMP = dados.TEMP_ATUAL;

    dados.TEMP_ATUAL = TERMOPAR.readCelsius();

    Serial.printf(
        "Temperatura: %.2f °C\n",
        dados.TEMP_ATUAL
    );
}

void atualizarSensores() {

  if (millis() - ultimaLeituraSensor >= 1000) {

    ultimaLeituraSensor = millis();

    lerTemperatura();

  }

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