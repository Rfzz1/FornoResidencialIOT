#include <Arduino.h>
#include "config.h"
#include "leds.h"
#include "telemetria.h"

static unsigned long milisPiscarLedErro = 0;
static unsigned long milisPiscarLedAguardandoConfigBluetooth = 0;
static unsigned long milisPiscarLedInicializacao = 0;

void desligarLEDs() {

  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);

}

void piscarLedErro() {

  if (millis() - milisPiscarLedErro >= 500) {

    milisPiscarLedErro = millis();

    dados.estadoPisca = !dados.estadoPisca;

    desligarLEDs();

    digitalWrite(RED, dados.estadoPisca);

  }
}

void piscarLedAguardandoConfigBluetooth() {
  
  if (millis() - milisPiscarLedAguardandoConfigBluetooth >= 500) {

    milisPiscarLedAguardandoConfigBluetooth = millis();

    dados.estadoPisca = !dados.estadoPisca;

    desligarLEDs();

    digitalWrite(GREEN, dados.estadoPisca);
    digitalWrite(RED, dados.estadoPisca);

  }
}

void piscarLedInicializacao() {

  if (millis() - milisPiscarLedInicializacao >= 500) {

    milisPiscarLedInicializacao = millis();

    dados.estadoPisca = !dados.estadoPisca;

    desligarLEDs();

    digitalWrite(RED, dados.estadoPisca);
    digitalWrite(GREEN, dados.estadoPisca);

  }
}

void atualizarLEDs(estadoSistema estadoAtual) {

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