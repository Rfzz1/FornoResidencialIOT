  #include <Arduino.h>
  #include <ArduinoJson.h>
  #include "config.h"
  #include "sensores.h"
  #include "leds.h"
  #include "buzzer.h"
  #include "api.h"
  #include "estados.h"
  #include "telemetria.h"
  #include "iot.h"
  #include "ws.h"
  #include "utils.h"
  #include "cerebroRTOS.h"
  #include "alertas.h"
  #include "api.h"
  #include "bluetooth.h"
 
  QueueHandle_t temperaturaQueue;
  QueueHandle_t eventosQueue;
  QueueHandle_t eventosFornoQueue;  
  SemaphoreHandle_t mutexEstadoForno;
  SemaphoreHandle_t mutexEstadoSistema;
  SemaphoreHandle_t mutexTelemetria;

  void setup() {

    Serial.begin(115200);
    Serial.println("FIRMWARE V2.0");

    inicializarPreferences();
    verificarEstadoDispositivo();

    //Filas RTOS

    temperaturaQueue = xQueueCreate(1, sizeof(double));
    eventosQueue = xQueueCreate(1, sizeof(eventoSistema));
    eventosFornoQueue = xQueueCreate(1, sizeof(estadoForno));
    mutexEstadoSistema = xSemaphoreCreateMutex();
    mutexEstadoForno = xSemaphoreCreateMutex();
    mutexTelemetria = xSemaphoreCreateMutex();

    //Tasks RTOS

    xTaskCreatePinnedToCore(taskConectividade, "Task IOT", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskCerebro, "Task Cerebro", 8192, NULL, 6, NULL, 1);
    xTaskCreatePinnedToCore(taskBluetooth, "Task Bluetooth", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskNuvem, "Task API", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskTemperatura, "Task Leitura", 2048, NULL, 6, NULL, 0);
    xTaskCreatePinnedToCore(taskAlertas, "Task Alertas", 2048, NULL, 4, NULL, 0);

    //Definição de Pinos

    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);
    pinMode(BUZZER, OUTPUT);

  }

  void loop() {


  }