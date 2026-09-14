  #include <Arduino.h>
  #include <ArduinoJson.h>
  #include <WebSocketsClient.h>
  #include "config.h"
  #include "sensores.h"
  #include "api.h"
  #include "estados.h"  
  #include "iot.h"
  #include "ws.h"
  #include "utils.h"
  #include "cerebroRTOS.h"
  #include "alertas.h"
  #include "api.h"
  #include "bluetooth.h"
  #include "sessao.h"
  #include "ws.h"

  //Websocket

  WebSocketsClient webSocket;

  //Filas e mutaxes
 
  QueueHandle_t temperaturaQueue;
  QueueHandle_t eventosQueue;
  QueueHandle_t eventosFornoQueue;  
  SemaphoreHandle_t mutexEstadoForno;
  SemaphoreHandle_t mutexEstadoSistema;
  SemaphoreHandle_t mutexTelemetria;
  SemaphoreHandle_t mutexWebSocket;

  //Preferences

  Preferences preferences;

  void setup() {

    //Inicialização da plca

    Serial.begin(115200);
    Serial.println("FIRMWARE V2.0");

    //Websockets

    webSocket.beginSSL("monitoramentoforno.com.br", 443, "/v1/ws/fornos");
    webSocket.onEvent(aoReceberEventoWebSocket);
    webSocket.setReconnectInterval(5000); // Tenta reconectar a cada 5 segundos

    //Bluetooth e provisionamento

    inicializarPreferences();
    verificarEstadoDispositivo();

    //Filas RTOS

    temperaturaQueue = xQueueCreate(1, sizeof(double));
    eventosQueue = xQueueCreate(3, sizeof(eventoSistema));
    eventosFornoQueue = xQueueCreate(3, sizeof(estadoForno));
    mutexEstadoSistema = xSemaphoreCreateMutex();
    mutexEstadoForno = xSemaphoreCreateMutex();
    mutexTelemetria = xSemaphoreCreateMutex();
    mutexWebSocket = xSemaphoreCreateMutex();

    //Definição de Pinos

    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);
    pinMode(BUZZER, OUTPUT);

    //Tasks RTOS

    xTaskCreatePinnedToCore(taskConectividade, "Task IOT", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskCerebro, "Task Cerebro", 8192, NULL, 6, NULL, 1);
    xTaskCreatePinnedToCore(taskBluetooth, "Task Bluetooth", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskNuvem, "Task API", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskTemperatura, "Task Leitura", 4096, NULL, 6, NULL, 0);
    xTaskCreatePinnedToCore(taskAlertas, "Task Alertas", 2048, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(taskSessao, "Task Sessao", 4096, NULL, 5, NULL, 0);
  
    Serial.println("TASKS INICIADAS COM SUCESSO!");
  }

  void loop() {

    webSocket.loop(); // Mantém o WebSocket ativo e gerencia reconexões

  }