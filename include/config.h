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

enum estadoSistema {
  INICIANDO,
  SEGURO,
  ALERTA,
  CRITICO,
  ERRO_SENSOR
};

enum estadoForno {
  FORNO_DESLIGADO,
  FORNO_AQUECENDO,
  FORNO_ATIVO,
  FORNO_ESFRIANDO
};

// =====================================================
// VARIÁVEIS GLOBAIS
// =====================================================

extern bool estadoPisca;
extern bool estadoBuzzer;
extern bool horaSincronizada;

extern float TEMP_ATUAL;
extern float ULTIMA_TEMP;
extern float tempoLigadoHoras;
extern float tempoLigadoMinutos;

extern unsigned long tempoLigado;
extern unsigned long tempoLigadoSegundos;

extern unsigned long milisInicioSessao;

extern unsigned long milisEstabilizarTermopar;
extern unsigned long milisLerTemp;

extern unsigned long milisPiscarLedErro;
extern unsigned long milisPiscarLedInicializacao;

extern unsigned long milisBuzzerErro;
extern unsigned long milisBuzzerAlerta;
extern unsigned long milisEnviarBlynk;

extern unsigned long milisEstabilizarWiFi;
extern unsigned long milisEstabilizarHoraLocal;
extern unsigned long milisAtualizarHoraLocal;

extern unsigned long tentativaNTP;
extern unsigned long inicioTentativa;

extern estadoSistema estadoAtual;

extern estadoForno estadoFornoAtual;
extern estadoForno estadoFornoAnterior;

extern char ssid[];
extern char pass[];

extern BlynkTimer timer;

extern int HoraInicio;
extern int MinutoInicio;

extern int HoraFim;
extern int MinutoFim;

extern int HorarioInicio;
extern int HorarioFim;

extern int TempoAlarme;
extern int HoraAtual;
extern int MinutoAtual;
extern int SegundoAtual;

#endif