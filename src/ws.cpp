#include "ws.h"
#include <Websockets.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "api.h"
#include "config.h"
#include "buzzer.h"
#include "telemetria.h"

void aoReceberEventoWebSocket(WStype_t tipoEvento, uint8_t * texto, size_t tamanho) {
    switch (tipoEvento) {
        case WStype_CONNECTED:
            Serial.println("[WS] Conectado ao servidor WebSocket.");
            break;
        case WStype_DISCONNECTED:
            Serial.println("[WS] Desconectado do servidor WebSocket.");
            break;
        case WStype_TEXT: {
            Serial.println("[WS] Mensagem de texto recebida.");

            JsonDocument doc;

            DeserializationError error = deserializeJson(doc, texto, tamanho);

            if (error) {
                Serial.print("[WS] Erro ao desserializar JSON: ");
                Serial.println(error.f_str());
                return;
            }

            const char* acao = doc["acao"];
            boolean muted = doc["muted"];

            if (acao && strcmp(acao, "MUTE") == 0 && muted) {
                Serial.println("[WS] Comando MUTE recebido!");
                dados.buzzerMutado = true;
                atualizarBuzzer(dados.estadoAtual);
            } else {
                Serial.println("[WS] Comando desconhecido ou inválido.");
            }
            break;
        }

        default:
            Serial.println("[WS] Evento desconhecido recebido.");
            break;
    }
}