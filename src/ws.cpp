#include <WebSocketsClient.h>
#include "ws.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "api.h"
#include "config.h"
#include "buzzer.h"
#include "telemetria.h"

  //Websocket

  WebSocketsClient webSocket;


void taskWebSocket(void *parameter) {

    webSocket.onEvent(aoReceberEventoWebSocket);
    bool fezLogin;
    String token;

    for (;;) {

        if (!WiFi.isConnected()) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            dados.iniciadoWebSocket = false;
            continue;
        }

        if (dados.iniciadoWebSocket == false) {

            xSemaphoreTake(mutexLoginWebSocket, portMAX_DELAY);
                fezLogin = dados.fezLogin;
                token = dados.tokenUsuario;
            xSemaphoreGive(mutexLoginWebSocket);

            if (fezLogin && !webSocket.isConnected()) {

                Serial.println("[WS] Tentando reconectar ao servidor WebSocket...");
                webSocket.setExtraHeaders("Origin: https://monitoramentoforno.com.br");
                webSocket.beginSSL("monitoramentoforno.com.br", 443, "/v1/ws/" + dados.serialNumber + "/fornos" + "?token=" + token, "");
                webSocket.enableHeartbeat(3000, 10000, 5); // Envia heartbeat a cada 3 segundos, timeout de 10 segundos, tenta reconectar 5 vezes
                webSocket.setReconnectInterval(5000); // Tenta reconectar a cada 5 segundos

                Serial.println("WebSocket iniciado com sucesso!");
                dados.iniciadoWebSocket = true;

            }
        }

        webSocket.loop();
        vTaskDelay(10 / portTICK_PERIOD_MS); // Pequena pausa para evitar sobrecarga da CPU
    }
}

void aoReceberEventoWebSocket(WStype_t tipoEvento, uint8_t * texto, size_t tamanho) {
    switch (tipoEvento) {
        case WStype_CONNECTED:
            Serial.println("[WS] Conectado ao servidor WebSocket.");
            break;
        case WStype_DISCONNECTED:
            Serial.println("[WS] Desconectado do servidor WebSocket.");
            dados.iniciadoWebSocket = false;
            break;
        case WStype_TEXT: {
            Serial.println("[WS] Mensagem de texto recebida.");
            Serial.printf("[WS] Texto recebido: %.*s\n", (int)tamanho, (char*)texto);

            JsonDocument doc;

            String textoRecebido((char*)texto, tamanho);

            if (textoRecebido.startsWith("{")) {

                DeserializationError error = deserializeJson(doc, texto, tamanho);

                if (error) {
                    Serial.print("[WS] Erro ao desserializar JSON: ");
                    Serial.println(error.f_str());
                    return;
                }

                const char* acao = doc["acao"];
                boolean muted = doc["muted"];
                uint32_t duracaoSegundos = doc["duracaoSegundos"];

                if (acao && strcmp(acao, "MUTE") == 0 && muted) {

                    Serial.println("[WS] Comando MUTE recebido!");

                    xSemaphoreTake(mutexWebSocket, portMAX_DELAY);
                        dados.buzzerMutado = true;
                    xSemaphoreGive(mutexWebSocket);

                } else if (acao && strcmp(acao, "DISPARAR") == 0 && duracaoSegundos){

                    Serial.println("[WS] Comando DISPARAR recebido!");

                    xSemaphoreTake(mutexTemporizador, portMAX_DELAY);
                        dados.temporizadorLigado = true;
                        dados.mensagemChegou = millis();
                        dados.duracaoSegundosTemporizador = duracaoSegundos;
                    xSemaphoreGive(mutexTemporizador);

                } else {
                    Serial.println("[WS] Comando desconhecido ou inválido.");
                }
            } else {
                Serial.println("[WS] Mensagem informativa recebida e ignorada");
            }

            break;
        }

        case WStype_PONG: {
            Serial.println("[WS] Pong recebido.");
            break;

        }

        default:
            Serial.println("[WS] Evento desconhecido recebido.");
            Serial.printf("[WS] Evento recebido: %i\n", (int)tipoEvento);
            break;
    }
}