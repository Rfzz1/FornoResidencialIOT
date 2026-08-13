  #include <Arduino.h>
  #include <ArduinoJson.h>
  #include "config.h"
  #include "sensores.h"
  #include "leds.h"
  #include "buzzer.h"
  #include "api.h"
  #include "telemetria.h"
  #include "iot.h"
  #include "ws.h"

  QueueHandle_t temperaturaQueue;

  void setup() {

    Serial.begin(115200);

    Serial.println("FIRMWARE V3");

    //Filas RTOS

    temperaturaQueue = xQueueCreate(1, sizeof(float));

    inicializarBluetooth();

    Serial.println("Aguardando comandos Bluetooth por 10 segundos...");

    unsigned long inicio = millis();

    while (millis() - inicio < 10000) {
        processarBluetooth();
        piscarLedAguardandoConfigBluetooth();
        delay(10);
    }

    inicializarPreferences();
    verificarEstadoDispositivo();

    dados.inicioTentativa = millis(); 

    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);

    pinMode(BUZZER, OUTPUT);


    dados.milisEstabilizarTermopar = millis();

  }

  void loop() {

    static unsigned long ultimoLoop = 0;

    if (millis() - ultimoLoop >= 1000) {

        ultimoLoop = millis();

        Serial.println("LOOP VIVO");
    }

    verificarReiniciar();
    gerenciarEstadoOperacional();

    if (dados.espConfigurado &&
        WiFi.status() == WL_CONNECTED) {

        estabilizarHoraLocal();
        processarWebSocket();
    }

    atualizarLEDs();
    atualizarBuzzer();

  }