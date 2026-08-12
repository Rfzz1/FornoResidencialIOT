#ifndef CONFIG_H
#define CONFIG_H

// =====================================================
// PINOS
// =====================================================
#define BLUE 23
#define GREEN 21
#define RED 22

#define TERMOPAR_SO 19
#define TERMOPAR_CS 5
#define TERMOPAR_SCK 18

#define BUZZER 15

#define TERMOSTATO 4

#include <Arduino.h>
#include <math.h>
#include <MAX6675.h>
#include <WiFi.h>

// =====================================================
// SENSOR
// =====================================================

extern MAX6675 TERMOPAR;
// =====================================================
// CONSTANTES
// =====================================================
constexpr const char* API_BASE_URL = "http://monitoramentoforno.com.br";

//protocolo "wss://" (WebSocket Seguro)
constexpr const char* WS_URL = "wss://www.monitoramentoforno.com.br/ws/forno?serialNumber=";

const String URL_EVENTOS = String(API_BASE_URL) + "/v1/eventos";
const float TEMP_CRITICA_ENTRADA = 300;
const float TEMP_CRITICA_SAIDA = 290;
const float TEMP_EXT_MAXIMA = 80;

const float TEMP_ALERTA_ENTRADA = 250;
const float TEMP_ALERTA_SAIDA = 240;

const float TEMP_MAXIMA_VALIDA = 400;

const float MARGEM_ESTABILIDADE = 2.0;

constexpr const char* ntpServer = "pool.ntp.org";
constexpr long  gmtOffset_sec = -10800; //CARLOS BARBOSA - GMT-3
constexpr int   daylightOffset_sec = 0;

const int TAMANHO_FILA_EVENTOS = 10;

// =====================================================
// ENUMS
// =====================================================

extern char ssid[];
extern char pass[];

#endif