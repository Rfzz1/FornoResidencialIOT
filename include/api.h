#ifndef API_H
#define API_H

#include <Arduino.h>

void fazerLogin();
bool iniciarSessao();
void encerrarSessao();
void enviarTemperatura();
void enviarEvento(String tipo);

String obterSerialNumber();

void processarBluetooth();
void inicializarBluetooth();
void inicializarPreferences();
void verificarEstadoDispositivo();
void diagnosticoCompleto();
void gerenciarEstadoOperacional();
void verificarReiniciar();
void sincronizarTemperaturas();
void salvarSecretBluetooth(String recebidoDoTerminal);

bool garantirLogin();

int enviarRequisicaoHTTP(
    const String &url,
    const String &metodo,
    const String &body,
    String *response = nullptr
);

#endif