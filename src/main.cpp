  #include <Arduino.h>
  #include <ArduinoJson.h>
  #include "config.h"
  #include "telemetria.h"
  #include "sensores.h"
  #include "api.h"
  #include "estados.h"  
  #include "iot.h"
  #include "ws.h"
  #include "utils.h"
  #include "cerebroRTOS.h"
  #include "alertas.h"
  #include "bluetooth.h"
  #include "sessao.h"

  //Filas e mutaxes
 
  QueueHandle_t temperaturaQueue;
  QueueHandle_t eventosQueue;
  QueueHandle_t eventosFornoQueue;  
  SemaphoreHandle_t mutexEstadoForno;
  SemaphoreHandle_t mutexEstadoSistema;
  SemaphoreHandle_t mutexTelemetria;
  SemaphoreHandle_t mutexWebSocket;
  SemaphoreHandle_t mutexTemporizador;
  SemaphoreHandle_t mutexLoginWebSocket;
  SemaphoreHandle_t mutexHttps;
  SemaphoreHandle_t mutexSessao;

  //Preferences

  Preferences preferences;

  void setup() {

    //Inicialização da plca

    Serial.begin(115200);
    Serial.println("FIRMWARE V2.0");

    // preferences.begin("forno-app", false);
    // preferences.clear(); // Apaga tudo o que está salvo na memória flash
    // preferences.end();

    //Bluetooth e provisionamento

    mutexLoginWebSocket = xSemaphoreCreateMutex();

    inicializarPreferences();
    verificarEstadoDispositivo();

    //Filas RTOS

    temperaturaQueue = xQueueCreate(1, sizeof(double));
    eventosQueue = xQueueCreate(5, sizeof(eventoSistema));
    eventosFornoQueue = xQueueCreate(3, sizeof(estadoForno));
    mutexEstadoSistema = xSemaphoreCreateMutex();
    mutexEstadoForno = xSemaphoreCreateMutex();
    mutexTelemetria = xSemaphoreCreateMutex();
    mutexWebSocket = xSemaphoreCreateMutex();
    mutexTemporizador = xSemaphoreCreateMutex();
    mutexHttps = xSemaphoreCreateMutex();
    mutexSessao = xSemaphoreCreateMutex();

    //Definição de Pinos

    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);
    pinMode(BUZZER, OUTPUT);

    //Tasks RTOS

    xTaskCreatePinnedToCore(taskConectividade, "Task IOT", 3072, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskCerebro, "Task Cerebro", 4096, NULL, 6, NULL, 1);
    xTaskCreatePinnedToCore(taskBluetooth, "Task Bluetooth", 3072, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskNuvem, "Task API", 8024, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskTemperatura, "Task Leitura", 2048, NULL, 6, NULL, 0);
    xTaskCreatePinnedToCore(taskAlertas, "Task Alertas", 2048, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(taskSessao, "Task Sessao", 6144, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(taskWebSocket, "Task WebSocket", 8024, NULL, 1, NULL, 1);
    Serial.println("TASKS INICIADAS COM SUCESSO!");
  }

  void loop() {

  }
