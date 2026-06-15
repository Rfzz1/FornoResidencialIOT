#ifndef CONFIG_H
#define CONFIG_H

// =====================================================
// PINOS
// =====================================================

#define RED 23
#define GREEN 22
#define BLUE 21

#define TERMOPAR_SO 19
#define TERMOPAR_CS 5
#define TERMOPAR_SCK 18

#define BUZZER 15

#define BLYNK_TEMPLATE_ID "TMPL2oYlTOGoQ"
#define BLYNK_TEMPLATE_NAME "Monitoramento de Forno"
#define BLYNK_AUTH_TOKEN "pzm-svahtHaKYCSybo0oVmMNX4uhM-qJ"

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

const float TEMP_CRITICA_ENTRADA = 300;
const float TEMP_CRITICA_SAIDA = 290;

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