#ifndef API_H
#define API_H

#include <Arduino.h>

void fazerLogin();
bool iniciarSessao();
void encerrarSessao();
void enviarTemperatura(const String& estadoFornoTexto, double temperaturaAtual, double temperaturaUltima);
void enviarEvento(String tipo);
void enviarTelemetria(double temperaturaAtual, double temperaturaUltima, const String& estadoSistema, const String& estadoForno, int tempoLigado);
void atualizarSessao();

String obterSerialNumber();

void processarBluetooth();
void inicializarBluetooth();
void inicializarPreferences();
void verificarEstadoDispositivo();
void diagnosticoCompleto();
void verificarReiniciar();
void salvarSecretBluetooth(String recebidoDoTerminal);

bool garantirLogin();

int enviarRequisicaoHTTP(
    const String &url,
    const String &metodo,
    const String &body,
    String *response,
    int tentativa
);

void taskNuvem(void *parameter);

#endif