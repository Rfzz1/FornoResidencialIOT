#include <WebSocketsClient.h>
#include "ws.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "api.h"
#include "config.h"
#include "buzzer.h"
#include "telemetria.h"
#include "utils.h"

#define INTERVALO_TELEMETRIA_WS_MS 1000   // dashboard (tabela telemetrias)
#define INTERVALO_TEMPERATURA_WS_MS 5000  // gráfico (tabela temperaturas). Máx. 720 linhas/hora; a API ainda descarta variação <= 1 °C
#define INTERVALO_SESSAO_WS_MS 10000      // estado da sessão (antes era PUT HTTPS)

// Websocket

WebSocketsClient webSocket;

// Estas Strings precisam viver enquanto o WebSocket existir:
// a biblioteca pode guardar ponteiros para elas.
static String wsHeaders;
static String wsPath;
static String wsTokenEmUso = "";
static bool wsConfigurado = false;
static String ultimoEstadoFornoEnviado = "";

// Um handshake TLS precisa de um bloco contíguo grande de heap.
static const uint32_t HEAP_MINIMO_HANDSHAKE = 32000;

static void publicarTelemetriaWS() {

    float atual, ultima;
    String estadoSistemaTexto, estadoFornoTexto;
    uint32_t tempoLigadoMin;

    xSemaphoreTake(mutexTelemetria, portMAX_DELAY);
        atual = condicoes.TEMP_ATUAL;
        ultima = condicoes.ULTIMA_TEMP;
        estadoSistemaTexto = obterEstadoSistemaTexto(condicoes.estadoAtual);
        estadoFornoTexto = obterEstadoFornoTexto(condicoes.estadoFornoAtual);
        tempoLigadoMin = condicoes.tempoLigadoMinutos;
    xSemaphoreGive(mutexTelemetria);

    if (isnan(atual)) return;

    // Desligado e o servidor já sabe: silêncio (mesmo comportamento da versão HTTP)
    if (estadoFornoTexto == "FORNO_DESLIGADO" && estadoFornoTexto == ultimoEstadoFornoEnviado) return;

    JsonDocument doc;
    doc["msg"] = "TELEMETRIA";
    doc["temperaturaAtual"] = atual;
    doc["temperaturaUltima"] = isnan(ultima) ? 0.0 : ultima;
    doc["estadoSistema"] = estadoSistemaTexto;
    doc["estadoForno"] = estadoFornoTexto;
    doc["tempoLigadoMinutos"] = tempoLigadoMin;

    String json;
    serializeJson(doc, json);

    if (webSocket.sendTXT(json)) ultimoEstadoFornoEnviado = estadoFornoTexto;
}

// Mesmo conteúdo do POST /v1/temperaturas, mas pela conexão já aberta (sem novo handshake TLS)
static void publicarTemperaturaWS() {

    String sessao = getSessaoId();
    if (sessao.isEmpty()) return;   // quem cria a sessão é a taskNuvem

    float atual, ultima;
    estadoForno estadoFornoAgora;

    xSemaphoreTake(mutexTelemetria, portMAX_DELAY);
        atual = condicoes.TEMP_ATUAL;
        ultima = condicoes.ULTIMA_TEMP;
        estadoFornoAgora = condicoes.estadoFornoAtual;
    xSemaphoreGive(mutexTelemetria);

    if (isnan(atual) || estadoFornoAgora == FORNO_DESLIGADO) return;

    JsonDocument doc;
    doc["msg"] = "TEMPERATURA";
    doc["sessaoId"] = sessao;
    doc["temperaturaAtual"] = atual;
    doc["temperaturaUltima"] = isnan(ultima) ? 0.0 : ultima;

    String json;
    serializeJson(doc, json);
    webSocket.sendTXT(json);
}

// Mesmo conteúdo do PUT /v1/sessoes/{id}/atualizar
static void publicarSessaoWS() {

    String sessao = getSessaoId();
    if (sessao.isEmpty()) return;

    String estadoSistemaTexto, estadoFornoTexto;

    xSemaphoreTake(mutexTelemetria, portMAX_DELAY);
        estadoSistemaTexto = obterEstadoSistemaTexto(condicoes.estadoAtual);
        estadoFornoTexto = obterEstadoFornoTexto(condicoes.estadoFornoAtual);
    xSemaphoreGive(mutexTelemetria);

    if (estadoFornoTexto == "FORNO_DESLIGADO") return;  // o encerramento é feito pela taskNuvem

    JsonDocument doc;
    doc["msg"] = "SESSAO";
    doc["sessaoId"] = sessao;
    doc["estadoSistemaAtual"] = estadoSistemaTexto;
    doc["estadoFornoAtual"] = estadoFornoTexto;

    String json;
    serializeJson(doc, json);
    webSocket.sendTXT(json);
}

// Chamada por QUALQUER task: só monta a mensagem e coloca na fila (não bloqueia, sem heap)
bool enviarEventoPeloWebSocket(const char *tipoEvento, const String &sessaoId) {

    if (!condicoes.wsConectado || sessaoId.isEmpty()) return false;

    MensagemWS msg;
    int n = snprintf(msg.payload, sizeof(msg.payload),
                     "{\"msg\":\"EVENTO\",\"tipo\":\"%s\",\"sessaoId\":\"%s\"}",
                     tipoEvento, sessaoId.c_str());

    if (n <= 0 || n >= (int)sizeof(msg.payload)) return false;   // não coube: HTTP
    return xQueueSend(wsEventosQueue, &msg, 0) == pdTRUE;        // fila cheia: HTTP
}

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
    // a PRÓPRIA biblioteca reconecta sozinha. Como o handshake agora não disputa
    // memória com o HTTPS (ver taskWebSocket), dá para tentar com mais frequência.
    webSocket.setReconnectInterval(5000);

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
            condicoes.wsConectado = false;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            condicoes.iniciadoWebSocket = false;
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

        if (webSocket.isConnected())
        {
            webSocket.loop();
        }
        else if (xSemaphoreTake(mutexHttps, 0) == pdTRUE)
        {
            // Desconectado: este loop() pode iniciar um handshake TLS.
            // Só deixamos isso acontecer quando NENHUM HTTPS está em andamento
            // (mesmo lock usado pelo HTTPS). Antes os dois handshakes rodavam ao
            // mesmo tempo, faltava memória ("BIGNUM - Memory allocation failed")
            // e o WebSocket nunca conectava com o forno ligado.
            webSocket.loop();
            xSemaphoreGive(mutexHttps);
        }

        if (webSocket.isConnected())
        {
            MensagemWS msg;
            while (xQueueReceive(wsEventosQueue, &msg, 0) == pdTRUE) {
                webSocket.sendTXT(msg.payload);
            }

            static unsigned long ultimoEnvio = 0;
            if (millis() - ultimoEnvio >= INTERVALO_TELEMETRIA_WS_MS) {
                ultimoEnvio = millis();
                publicarTelemetriaWS();
            }

            static unsigned long ultimoEnvioTemperatura = 0;
            if (millis() - ultimoEnvioTemperatura >= INTERVALO_TEMPERATURA_WS_MS) {
                ultimoEnvioTemperatura = millis();
                publicarTemperaturaWS();
            }

            static unsigned long ultimoEnvioSessao = 0;
            if (millis() - ultimoEnvioSessao >= INTERVALO_SESSAO_WS_MS) {
                ultimoEnvioSessao = millis();
                publicarSessaoWS();
            }
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
        condicoes.wsConectado = true;
        ultimoEstadoFornoEnviado = "";
        break;

    case WStype_DISCONNECTED:
        // NÃO reiniciar a conexão aqui: a biblioteca já tenta reconectar sozinha.
        // Reiniciar manualmente criava handshakes duplicados e esgotava o heap.
        // A biblioteca manda o motivo quando o servidor recusa o handshake
        // (ex.: "HTTP 401", "WebSocket handshake failed - HTTP 500"). Sem texto
        // costuma ser queda da conexão ou falta de resposta ao ping (heartbeat).
        if (tamanho > 0)
        {
            Serial.printf("[WS] Desconectado. Motivo: %.*s | maiorBloco=%u\n",
                          (int)tamanho, (char *)texto, ESP.getMaxAllocHeap());
        }
        else
        {
            Serial.printf("[WS] Desconectado (sem motivo informado: queda ou sem pong) | maiorBloco=%u\n",
                          ESP.getMaxAllocHeap());
        }
        condicoes.wsConectado = false;
        condicoes.iniciadoWebSocket = false;
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