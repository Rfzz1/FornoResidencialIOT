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

#define MODO_DEBUG
#define MODO_INSEGURO

#define DURACAO_APITO_TEMPORIZADOR_SEG 15

#include <Arduino.h>
#include <math.h>
#include <MAX6675.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <Preferences.h>

// =====================================================
// SENSOR
// =====================================================

extern MAX6675 TERMOPAR;
// =====================================================
// CONSTANTES
// =====================================================

constexpr const char* API_BASE_URL = "https://monitoramentoforno.com.br";

// protocolo "wss://" (WebSocket Seguro)
constexpr const char *WS_URL = "wss://www.monitoramentoforno.com.br/ws/forno?serialNumber=";

const float TEMP_CRITICA_ENTRADA = 300;
const float TEMP_CRITICA_SAIDA = 290;

const float TEMP_ALERTA_ENTRADA = 250;
const float TEMP_ALERTA_SAIDA = 240;

const float TEMP_MAXIMA_VALIDA = 400;

// ---- Estado do forno (ver definirEstadoForno em estados.cpp) ----
// A cada leitura (1 s) compara a temperatura de agora com a de N segundos atrás.
const float TEMP_FORNO_LIGADO = 60;          // abaixo disso: FORNO_DESLIGADO
const uint8_t JANELA_TENDENCIA_SEG = 10;     // N: quantos segundos atrás comparar
const float MARGEM_ENTRAR_TENDENCIA = 2.0;   // variação (°C) para ENTRAR em AQUECENDO/ESFRIANDO
const float MARGEM_SAIR_TENDENCIA = 1.0;     // variação (°C) abaixo da qual VOLTA para ATIVO (evita oscilar)

constexpr long gmtOffset_sec = -10800; // CARLOS BARBOSA - GMT-3
constexpr int daylightOffset_sec = 0;

//FreeRTOS

extern QueueHandle_t temperaturaQueue;
extern QueueHandle_t eventosQueue;
extern QueueHandle_t eventosFornoQueue;
extern QueueHandle_t wsEventosQueue;

extern SemaphoreHandle_t mutexEstadoForno;
extern SemaphoreHandle_t mutexEstadoSistema;
extern SemaphoreHandle_t mutexTelemetria;
extern SemaphoreHandle_t mutexWebSocket;
extern SemaphoreHandle_t mutexTemporizador;
extern SemaphoreHandle_t mutexLoginWebSocket;
extern SemaphoreHandle_t mutexHttps;
extern SemaphoreHandle_t mutexSessao;

// Preferences

extern Preferences preferences;

// =====================================================
// ENUMS
// =====================================================

extern char ssid[];
extern char pass[];

#endif