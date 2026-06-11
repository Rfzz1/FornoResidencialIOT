#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include <BlynkSimpleEsp32.h>
#include "iot.h"
#include "logs.h"
#include <time.h>
#include "telemetria.h"

char ssid[] = "SENAI-TurmaTI_5G";
char pass[] = "SenaiGaribaldiCeit";
static unsigned long milisAtualizarHoraLocal = 0;
static unsigned long milisAtualizarHorarioAlarme = 0;
static unsigned long ultimoEnvioBlynk = 0;
static unsigned long milisEstabilizarWiFi = 0;
static unsigned long tentativaNTP = 0;

void iniciarBlynk() {
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect();
}

void conectarWiFi() {
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - milisEstabilizarWiFi >= 1000) {
            milisEstabilizarWiFi = millis();
            Serial.print(".");
        }
    }
    Serial.println("\nWiFi conectado.");

    configurarNTP();
    printLocalTime();
}

void conectarBlynk() {
    if (WiFi.status() == WL_CONNECTED) {

        if (!Blynk.connected()) {

            Blynk.connect(1000);

        }

        Blynk.run();
    } else {
        Serial.println("WiFi desconectado. Tentando reconectar...");
        conectarWiFi();
    }
}

void configurarNTP() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void estabilizarHoraLocal() {

    struct tm timeinfo;

    if (dados.horaSincronizada) {
        return;
    }


    if (millis() - tentativaNTP >= 1000) {
        tentativaNTP = millis();
    
        if (getLocalTime(&timeinfo)) {

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

void enviarBlynk() {
    Blynk.virtualWrite(V0, dados.TEMP_ATUAL);
    Blynk.virtualWrite(V1, obterEstadoFornoTexto());
    Blynk.virtualWrite(V2, obterEstadoSistemaTexto(dados.estadoAtual));
    Blynk.virtualWrite(V3, dados.tempoLigadoMinutos);
}


BLYNK_WRITE(V5) {
    TimeInputParam t(param);

    if (t.hasStartTime()) {
        dados.HoraInicio = t.getStartHour();
        dados.MinutoInicio = t.getStartMinute();
        dados.HoraFim = t.getStopHour();
        dados.MinutoFim = t.getStopMinute();
        dados.HorarioInicio = dados.HoraInicio * 60 + dados.MinutoInicio;
        dados.HorarioFim = dados.HoraFim * 60 + dados.MinutoFim;

        dados.TempoAlarme = dados.HorarioFim - dados.HorarioInicio;
        if (dados.TempoAlarme < 0) {
            dados.TempoAlarme += 24 * 60;
        }
    }
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

void atualizarEnvioBlynk() {
    if (millis() - ultimoEnvioBlynk >= 1000) {
        ultimoEnvioBlynk = millis();
        enviarBlynk();
    }
}