#include "ws.h"
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "api.h"

#include "config.h"      // Para pegar a URL base, etc.
#include "telemetria.h"  // Para acessar a variável 'dados'

using namespace websockets;
WebsocketsClient wsClient;

// Variável local para controle de reconexão
unsigned long ultimaTentativaWs = 0;

void onMessageCallback(WebsocketsMessage message) {
    Serial.print("[WS] Mensagem recebida: ");
    Serial.println(message.data());

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message.data());

    if (!error) {
        const char* acao = doc["acao"];

        if (acao && strcmp(acao, "MUTE") == 0) {
            Serial.println("[WS] Comando MUTE recebido!");
            
            // Em vez de mexer no hardware aqui, mudamos o ESTADO na telemetria
            dados.buzzerMutado = true; 
        }
    } else {
        Serial.print("[WS] Erro no JSON: ");
        Serial.println(error.f_str());
    }
}

void inicializarWebSocket() {
    Serial.println("[WS] Inicializando conexão segura...");
    wsClient.setInsecure(); // Ignora validação SSL
    wsClient.onMessage(onMessageCallback);
    String urlCompleta = String(WS_URL) + obterSerialNumber(); // Função fictícia para obter o Serial Number do dispositivo
    
    if (wsClient.connect(urlCompleta)) {
        Serial.println("[WS] Conectado com sucesso!");
    } else {
        Serial.println("[WS] Falha na conexão inicial.");
    }
}

void processarWebSocket() {
    // Só tenta processar se o Wi-Fi estiver ok
    if (WiFi.status() != WL_CONNECTED) return;

    if (wsClient.available()) {
        wsClient.poll();
    } else {
        // Tenta reconectar a cada 5 segundos se cair
        if (millis() - ultimaTentativaWs > 5000) {
            ultimaTentativaWs = millis();
            Serial.println("[WS] Tentando reconectar...");
            
            String urlCompleta = String(WS_URL) + obterSerialNumber();
            wsClient.connect(urlCompleta);
        }
    }
}