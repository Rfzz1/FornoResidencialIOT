#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <Arduino.h>

void taskBluetooth(void *parameter);
void inicializarBluetooth();
void inicializarPreferences();
void processarBluetooth();
void solicitarDesligamentoBluetooth();
void verificarReiniciar();

#endif