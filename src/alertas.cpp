#include <Arduino.h>
#include "config.h"
#include "alertas.h"

void alertas() {

  tempoLigadoHoras = tempoLigadoSegundos / 3600.0;
  tempoLigadoMinutos = (tempoLigadoSegundos % 3600) / 60.0;

  if (tempoLigadoHoras >= 1.5 &&
      TEMP_ATUAL >= 200) {

    Serial.println("Notificacao: Talvez voce tenha esquecido sua comida no forno!");

  }
}