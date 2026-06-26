#include <Arduino.h>
#include "config.h"
#include "alertas.h"
#include "telemetria.h"

static unsigned long milisAtualizarAlertas = 0;

void alertas() {

  dados.tempoLigadoHoras = dados.tempoLigadoSegundos / 3600.0;
  dados.tempoLigadoMinutos = (dados.tempoLigadoSegundos % 3600) / 60.0;

  if (dados.tempoLigadoHoras >= 1.5 &&
      dados.TEMP_ATUAL >= 200) {

    Serial.println("Notificacao: Talvez voce tenha esquecido sua comida no forno!");

  }

  if (dados.TEMP_EXT_ATUAL >= TEMP_EXT_MAXIMA) {
    Serial.println("Notificacao: Temperatura externa atingiu 80ºC!");
  }
}

void atualizarAlertas() {
  if (millis() - milisAtualizarAlertas >= 1000) {
    milisAtualizarAlertas = millis();
    alertas();
  }
}