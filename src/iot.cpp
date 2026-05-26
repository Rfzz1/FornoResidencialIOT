#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "iot.h"
#include "config.h"
#include "logs.h"
#include <time.h>

char ssid[] = "SENAI-TurmaTI_5G";
char pass[] = "SenaiGaribaldiCeit";
BlynkTimer timer;
bool horaSincronizada = false;
unsigned long ultimoEnvioBlynk = 0;
unsigned long milisAtualizarHoraLocal = 0;
unsigned long milisEstabilizarWiFi = 0;
unsigned long milisAtualizarHorarioAlarme = 0;

void conectarWiFi() {
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - milisEstabilizarWiFi >= 1000) {
            milisEstabilizarWiFi = millis();
            Serial.println(".");
        }
    }
    Serial.println("\nWiFi conectado.");

    configurarNTP();
    printLocalTime();
}

void configurarNTP() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void estabilizarHoraLocal() {

    tentativaNTP = 0;
    inicioTentativa = millis();

    struct tm timeinfo;

    if (horaSincronizada) {
        return;
    }


    if (millis() - tentativaNTP >= 1000) {
        tentativaNTP = millis();
    
        if (getLocalTime(&timeinfo)) {

            horaSincronizada = true;
            Serial.println("Hora local sincronizada.");

        } else {

            Serial.println("Obtendo hora local...");
        }
    }
    if (millis() - inicioTentativa >= 10000 && !horaSincronizada) {

        Serial.println("Falha ao sincronizar NTP.");
    }
}

void printLocalTime() {
    struct tm infoHoraLocal;

    if (getLocalTime(&infoHoraLocal) && millis() - milisAtualizarHoraLocal >= 1000) {
        milisAtualizarHoraLocal = millis();

        HoraAtual = infoHoraLocal.tm_hour;
        MinutoAtual = infoHoraLocal.tm_min;
        SegundoAtual = infoHoraLocal.tm_sec;

        Serial.printf(
            "%02d:%02d:%02d\n",
            HoraAtual,
            MinutoAtual,
            SegundoAtual
        );
    }
}

void enviarBlynk() {
    Blynk.virtualWrite(V0, TEMP_ATUAL);
    Blynk.virtualWrite(V1, obterEstadoFornoTexto());
    Blynk.virtualWrite(V2, obterEstadoSistemaTexto());
    Blynk.virtualWrite(V3, tempoLigadoMinutos);
}


BLYNK_WRITE(V5) {
    TimeInputParam t(param);

    if (t.hasStartTime()) {
        HoraInicio = t.getStartHour();
        MinutoInicio = t.getStartMinute();
        HoraFim = t.getStopHour();
        MinutoFim = t.getStopMinute();
        HorarioInicio = HoraInicio * 60 + MinutoInicio;
        HorarioFim = HoraFim * 60 + MinutoFim;

        TempoAlarme = HorarioFim - HorarioInicio;
        if (TempoAlarme < 0) {
            TempoAlarme += 24 * 60;
        }
    }
}

void verificarHorarioAlarme() {
    if (HoraAtual == HoraFim && MinutoAtual == MinutoFim) {
            estadoBuzzer = true;
        } else {
            estadoBuzzer = false;
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