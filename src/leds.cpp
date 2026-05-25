#include <Arduino.h>
#include "config.h"
#include "leds.h"

bool estadoPisca = false;

unsigned long milisPiscarLedErro = 0;
unsigned long milisPiscarLedInicializacao = 0;

void desligarLEDs() {

  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);

}

void piscarLedErro() {

  if (millis() - milisPiscarLedErro >= 500) {

    milisPiscarLedErro = millis();

    estadoPisca = !estadoPisca;

    desligarLEDs();

    digitalWrite(RED, estadoPisca);

  }
}

void piscarLedInicializacao() {

  if (millis() - milisPiscarLedInicializacao >= 500) {

    milisPiscarLedInicializacao = millis();

    estadoPisca = !estadoPisca;

    desligarLEDs();

    digitalWrite(RED, estadoPisca);
    digitalWrite(GREEN, estadoPisca);

  }
}

void atualizarLEDs() {

  switch (estadoAtual) {

    case INICIANDO:

      piscarLedInicializacao();

      break;

    case SEGURO:

      desligarLEDs();

      digitalWrite(GREEN, HIGH);

      break;

    case ALERTA:

      desligarLEDs();

      digitalWrite(RED, HIGH);
      digitalWrite(GREEN, HIGH);

      break;

    case CRITICO:

      desligarLEDs();

      digitalWrite(RED, HIGH);

      break;

    case ERRO_SENSOR:

      piscarLedErro();

      break;
  }
}