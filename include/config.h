#ifndef CONFIG_H
#define CONFIG_H

// =====================================================
// PINOS
// =====================================================

#define RED 25
#define GREEN 26
#define BLUE 27

#define TERMOPAR_SO 19
#define TERMOPAR_CS 5
#define TERMOPAR_SCK 18

#define BUZZER 23

#define BLYNK_TEMPLATE_ID "TMPL2oYlTOGoQ"
#define BLYNK_TEMPLATE_NAME "Monitoramento de Forno"
#define BLYNK_AUTH_TOKEN "pzm-svahtHaKYCSybo0oVmMNX4uhM-qJ"

#include <Arduino.h>
#include <math.h>
#include <MAX6675.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h> 

// =====================================================
// SENSOR
// =====================================================

extern MAX6675 TERMOPAR;
// =====================================================
// CONSTANTES
// =====================================================

const float TEMP_CRITICA_ENTRADA = 250;
const float TEMP_CRITICA_SAIDA = 240;

const float TEMP_ALERTA_ENTRADA = 200;
const float TEMP_ALERTA_SAIDA = 190;

const float TEMP_MAXIMA_VALIDA = 400;

const float MARGEM_ESTABILIDADE = 2.0;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800; //CARLOS BARBOSA - GMT-3
const int   daylightOffset_sec = 0;

// =====================================================
// ENUMS
// =====================================================

char ssid[];
char pass[];

#endif