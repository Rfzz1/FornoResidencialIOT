#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "iot.h"
#include "logs.h"
#include <time.h>
#include "telemetria.h"

char ssid[] = "A54 de Rafael";
char pass[] = "Rafa130209@";
static unsigned long milisAtualizarHoraLocal = 0;
static unsigned long milisAtualizarHorarioAlarme = 0;
static unsigned long milisEstabilizarWiFi = 0;
static unsigned long tentativaNTP = 0;
unsigned long ultimoTesteWiFi = 0;

void conectarWiFi() {
    WiFi.begin(ssid, pass);

    Serial.println("Conectando...");

    unsigned long inicioTentativa = millis();
    const unsigned long TIMEOUT_WIFI = 15000;

    while (WiFi.status() != WL_CONNECTED) {

        if(millis() - inicioTentativa >= TIMEOUT_WIFI) {
            Serial.println("Timeout ao conectar Wi-Fi. Operando offline.");
            return;
        }

        if (millis() - milisEstabilizarWiFi >= 1000) {
            milisEstabilizarWiFi = millis();
            Serial.print(".");
        }
    }
    Serial.println("\nWiFi cdronectado.");

    Serial.print("Status: ");
    Serial.println(WiFi.status());

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());

    configurarNTP();
    printLocalTime();
}

void verificarWiFi() {

    if (millis() - ultimoTesteWiFi < 5000)
        return;

    ultimoTesteWiFi = millis();

    if (WiFi.status() == WL_CONNECTED)
        return;

    Serial.println("Reconectando...");

    WiFi.disconnect();

    WiFi.begin(ssid, pass);

}

void configurarNTP() {
    configTime(
    -10800,
    0,
    "time.google.com",
    "time.cloudflare.com",
    "pool.ntp.org"
);
}

void estabilizarHoraLocal() {

    struct tm timeinfo;

    if (dados.horaSincronizada) {
        return;
    }


    if (millis() - tentativaNTP >= 1000) {
        tentativaNTP = millis();
    
        if (getLocalTime(&timeinfo, 10000)) {

            dados.horaSincronizada = true;
            Serial.println("Hora local sincronizada.");

        } else {

            Serial.println("Obtendo hora local...");
        }
    }
    if (millis() - dados.inicioTentativa >= 10000 && !dados.horaSincronizada && !dados.timeoutNTP) {

        dados.timeoutNTP = true;
        Serial.println("Falha ao sincronizar NTP.");
    }
}

void printLocalTime() {
    struct tm infoHoraLocal;

    if (getLocalTime(&infoHoraLocal) && millis() - milisAtualizarHoraLocal >= 1000) {
        milisAtualizarHoraLocal = millis();

        dados.HoraAtual = infoHoraLocal.tm_hour;
        dados.MinutoAtual = infoHoraLocal.tm_min;
        dados.SegundoAtual = infoHoraLocal.tm_sec;

        Serial.printf(
            "%02d:%02d:%02d\n",
            dados.HoraAtual,
            dados.MinutoAtual,
            dados.SegundoAtual
        );
    }
}

String obterHorarioFormatado() {

    char buffer[20];

    sprintf(
        buffer,
        "%02d:%02d:%02d",
        dados.HoraAtual,
        dados.MinutoAtual,
        dados.SegundoAtual
    );

    return String(buffer);
}

void verificarHorarioAlarme() {
    if (dados.HoraAtual == dados.HoraFim && dados.MinutoAtual == dados.MinutoFim) {
            dados.buzzerAtivaHorario = true;
        } else {
            dados.buzzerAtivaHorario = false;
        }
}

void atualizarHorarioAlarme() {
    if (millis() - milisAtualizarHorarioAlarme >= 1000) {
        milisAtualizarHorarioAlarme = millis();
        verificarHorarioAlarme();
    }
}