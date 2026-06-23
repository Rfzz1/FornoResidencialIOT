#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "sensores.h"
#include "telemetria.h"
#include "api.h"

static unsigned long ultimaLeituraSensor = 0;
static unsigned long ultimoEnvioTemperatura = 0;
static float leituraBruta = 0;

MAX6675 TERMOPAR(TERMOPAR_SCK, TERMOPAR_CS, TERMOPAR_SO);

void lerTemperatura() {

    dados.ULTIMA_TEMP = dados.TEMP_ATUAL;
    
    leituraBruta = TERMOPAR.readCelsius();

    if(dados.TEMP_ATUAL == 0) {
      dados.TEMP_ATUAL = leituraBruta;
      return;
    }

    if(!isnan(leituraBruta)) {
        dados.TEMP_ATUAL = (dados.TEMP_ATUAL * 0.7) + (leituraBruta * 0.3);
    } else {
        dados.TEMP_ATUAL = leituraBruta;
    }

    Serial.printf("Temperatura Filtrada: %.2f °C (Bruta: %.2f °C)\n", dados.TEMP_ATUAL, leituraBruta);
}

void atualizarSensores() {

  if (millis() - ultimaLeituraSensor >= 1000) {

    ultimaLeituraSensor = millis();
    lerTemperatura();
  }

  if(millis() - ultimoEnvioTemperatura >= 5000) {

    ultimoEnvioTemperatura = millis();

    if (temperaturaValida()) {
        sincronizarTemperaturas();
    } else {
        // Se a temperatura não for válida, podemos optar por não enviar ou enviar um valor específico
        Serial.println("Temperatura inválida. Não enviando dados para a API.");
    }

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