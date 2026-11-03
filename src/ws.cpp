#include <WebSocketsClient.h>
#include "ws.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "api.h"
#include "config.h"
#include "buzzer.h"
#include "telemetria.h"
<<<<<<< HEAD
#include "certs.h"
=======
>>>>>>> a1f89155ddf57c6931674e7c4a4287d62a3eec2f

// Websocket

WebSocketsClient webSocket;

// Estas Strings precisam viver enquanto o WebSocket existir:
// a biblioteca pode guardar ponteiros para elas.
static String wsHeaders;
static String wsPath;
static String wsTokenEmUso = "";
static bool wsConfigurado = false;

// Um handshake TLS precisa de um bloco contíguo grande de heap.
static const uint32_t HEAP_MINIMO_HANDSHAKE = 32000;

static void configurarWebSocket(const String &token)
{
    // Se já existia uma conexão (token novo), fecha antes de reconfigurar
    if (wsConfigurado)
    {
        webSocket.disconnect();
    }

    wsHeaders = "Authorization: Bearer " + token + "\r\nOrigin: https://monitoramentoforno.com.br";
    wsPath = "/v1/ws/" + condicoes.serialNumber + "/fornos";

    webSocket.setExtraHeaders(wsHeaders.c_str());
#ifdef MODO_INSEGURO
    webSocket.beginSSL("monitoramentoforno.com.br", 443, wsPath.c_str(), "");
#else
    webSocket.beginSslWithCA("monitoramentoforno.com.br", 443, wsPath.c_str(), ROOT_CA);
#endif

    // ping a cada 15s, espera pong por 5s, desiste após 2 falhas seguidas
    webSocket.enableHeartbeat(15000, 5000, 2);
    webSocket.setReconnectInterval(15000); // a PRÓPRIA biblioteca reconecta a cada 15s

    wsTokenEmUso = token;
    wsConfigurado = true;
    condicoes.iniciadoWebSocket = true;

    Serial.println("[WS] Configurado. A biblioteca cuida das reconexoes.");
}

void taskWebSocket(void *parameter)
{
    webSocket.onEvent(aoReceberEventoWebSocket);

    unsigned long ultimoAvisoHeap = 0;

    for (;;)
    {
        if (!WiFi.isConnected())
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        bool fezLogin;
        String token;

        xSemaphoreTake(mutexLoginWebSocket, portMAX_DELAY);
        fezLogin = condicoes.fezLogin;
        token = condicoes.tokenUsuario;
        xSemaphoreGive(mutexLoginWebSocket);

        if (!fezLogin || token.isEmpty())
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            continue;
        }

        // Configura UMA vez, ou de novo só se o token mudou (novo login)
        if (!wsConfigurado || token != wsTokenEmUso)
        {
            configurarWebSocket(token);
        }

<<<<<<< HEAD
        // Sem lock: o WebSocket nunca bloqueia o envio HTTP.
        // Só evita tentar o handshake TLS quando o heap está muito baixo.
        if (webSocket.isConnected() || ESP.getMaxAllocHeap() >= HEAP_MINIMO_HANDSHAKE)
        {
            webSocket.loop();
        }
        else if (millis() - ultimoAvisoHeap > 10000)
        {
            ultimoAvisoHeap = millis();
            Serial.printf("[WS] Heap baixo para handshake (maior bloco=%u). Aguardando.\n", ESP.getMaxAllocHeap());
=======
            if (fezLogin && !webSocket.isConnected())
            {

                Serial.println("[WS] Tentando reconectar ao servidor WebSocket...");
                webSocket.setExtraHeaders("Origin: https://monitoramentoforno.com.br");
                String urlWebSocket = "/v1/ws/" + condicoes.serialNumber + "/fornos?token=" + token;
                webSocket.beginSslWithCA("monitoramentoforno.com.br", 443, urlWebSocket.c_str(), ROOT_CA);
                webSocket.enableHeartbeat(3000, 10000, 5); // Envia heartbeat a cada 3 segundos, timeout de 10 segundos, tenta reconectar 5 vezes
                webSocket.setReconnectInterval(5000);      // Tenta reconectar a cada 5 segundos

                Serial.println("WebSocket iniciado com sucesso!");
                condicoes.iniciadoWebSocket = true;
            }
>>>>>>> a1f89155ddf57c6931674e7c4a4287d62a3eec2f
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); // Pequena pausa para evitar sobrecarga da CPU
    }
}

void aoReceberEventoWebSocket(WStype_t tipoEvento, uint8_t *texto, size_t tamanho)
{
    switch (tipoEvento)
    {
    case WStype_CONNECTED:
        Serial.println("[WS] Conectado ao servidor WebSocket.");
        break;

    case WStype_DISCONNECTED:
        // NÃO reiniciar a conexão aqui: a biblioteca já tenta reconectar sozinha.
        // Reiniciar manualmente criava handshakes duplicados e esgotava o heap.
        Serial.println("[WS] Desconectado do servidor WebSocket.");
        break;

    case WStype_TEXT:
    {
        Serial.println("[WS] Mensagem de texto recebida.");
        Serial.printf("[WS] Texto recebido: %.*s\n", (int)tamanho, (char *)texto);

        JsonDocument doc;

        String textoRecebido((char *)texto, tamanho);

        if (textoRecebido.startsWith("{"))
        {
            DeserializationError error = deserializeJson(doc, texto, tamanho);

            if (error)
            {
                Serial.print("[WS] Erro ao desserializar JSON: ");
                Serial.println(error.f_str());
                return;
            }

            const char *acao = doc["acao"];
            bool muted = doc["muted"];
            uint32_t duracaoSegundos = doc["duracaoSegundos"];

            if (acao && strcmp(acao, "MUTE") == 0 && muted)
            {
                Serial.println("[WS] Comando MUTE recebido!");

                xSemaphoreTake(mutexWebSocket, portMAX_DELAY);
                condicoes.buzzerMutado = true;
                xSemaphoreGive(mutexWebSocket);
            }
            else if (acao && strcmp(acao, "DISPARAR") == 0 && duracaoSegundos)
            {
                Serial.println("[WS] Comando DISPARAR recebido!");

                xSemaphoreTake(mutexTemporizador, portMAX_DELAY);
                condicoes.temporizadorLigado = true;
                condicoes.inicioDisparo = millis();
                xSemaphoreGive(mutexTemporizador);
            }
            else
            {
                Serial.println("[WS] Comando desconhecido ou invalido.");
            }
        }
        else
        {
            Serial.println("[WS] Mensagem informativa recebida e ignorada");
        }

        break;
    }

    case WStype_PONG:
        Serial.println("[WS] Pong recebido.");
        break;

    case WStype_PING:
        break;

    case WStype_ERROR:
        Serial.printf("[WS] Erro: %.*s\n", (int)tamanho, (char *)texto);
        break;

    default:
        Serial.printf("[WS] Evento desconhecido recebido: %i\n", (int)tipoEvento);
        break;
    }
}
