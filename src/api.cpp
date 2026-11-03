#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>

#include "config.h"
#include "telemetria.h"
#include "iot.h"
#include "estados.h"
#include "sessao.h"
#include "alertas.h"
#include "sensores.h"
#include "api.h"
#include "utils.h"
#include "bluetooth.h"

// ==========================
// OBJETOS GLOBAIS
// ==========================

String tokenJWT = "";
String sessaoId = "";

String getSessaoId()
{
    xSemaphoreTake(mutexSessao, portMAX_DELAY);
    String copia = sessaoId;
    xSemaphoreGive(mutexSessao);
    return copia;
}

void setSessaoId(String SessaoId)
{
    xSemaphoreTake(mutexSessao, portMAX_DELAY);
    sessaoId = SessaoId;
    xSemaphoreGive(mutexSessao);
}

String getTokenJWT()
{
    xSemaphoreTake(mutexLoginWebSocket, portMAX_DELAY);
    String copia = tokenJWT;
    xSemaphoreGive(mutexLoginWebSocket);
    return copia;
}

void setTokenJWT(String TokenJWT)
{
    xSemaphoreTake(mutexLoginWebSocket, portMAX_DELAY);
    tokenJWT = TokenJWT;
    xSemaphoreGive(mutexLoginWebSocket);
}

double temperaturaAtual = 0;
double temperaturaUltima = 0;

// Task para enviar telemetria periodicamente
void taskNuvem(void *parameter)
{

    double temperaturaAtualQueue;

    double temperaturaUltimaQueue;

    String estadoSistemaTexto = "";

    String estadoFornoTexto = "";

    uint32_t tempoLigadoMin = 0;
    static estadoForno estadoFornoAnterior = condicoes.estadoFornoAnterior;

    eventoSistema evento;

    estadoForno eventoForno;

    for (;;)
    {
        tratarTempoSessao();

        xSemaphoreTake(mutexTelemetria, portMAX_DELAY);

        temperaturaAtualQueue = condicoes.TEMP_ATUAL;

        temperaturaUltimaQueue = condicoes.ULTIMA_TEMP;

        estadoSistemaTexto = obterEstadoSistemaTexto(condicoes.estadoAtual);

        estadoFornoTexto = obterEstadoFornoTexto(condicoes.estadoFornoAtual);

        tempoLigadoMin = condicoes.tempoLigadoMinutos;

        xSemaphoreGive(mutexTelemetria);

        enviarTelemetria(temperaturaAtualQueue, temperaturaUltimaQueue, estadoSistemaTexto, estadoFornoTexto, tempoLigadoMin);

        enviarTemperatura(estadoFornoTexto, temperaturaAtualQueue, temperaturaUltimaQueue);

        while (xQueueReceive(eventosQueue, &evento, 0) == pdTRUE)
        {
            if (evento == NENHUM) continue;

            if (!enviarEvento(obterEventoSistemaTexto(evento)))
            {
                xQueueSendToFront(eventosQueue, &evento, 0); // devolve e tenta no próximo ciclo
                break;                                       // não adianta insistir agora
            }
        }

        while (xQueueReceive(eventosFornoQueue, &eventoForno, 0) == pdTRUE)
        {

            if (estadoFornoAnterior == FORNO_DESLIGADO && eventoForno != FORNO_DESLIGADO)
            {
                iniciarSessao();
            }

            if (eventoForno == FORNO_DESLIGADO && estadoFornoAnterior != FORNO_DESLIGADO)
            {

                Serial.println("Enviando telemetria final antes de encerrar...");

                enviarTelemetria(temperaturaAtualQueue, temperaturaUltimaQueue, estadoSistemaTexto, "FORNO_DESLIGADO", tempoLigadoMin);

                Serial.println("Encerrar Sessão - HTTP");
                encerrarSessao();
            }

            estadoFornoAnterior = eventoForno;
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

#ifdef MODO_DEBUG

void diagnosticoCompleto()
{

    Serial.printf("Heap livre: %d bytes\n", ESP.getFreeHeap());

    Serial.println("\n===== INICIANDO DIAGNOSTICO =====");

    // ---------- TESTE 1: DNS ----------
    Serial.println("\n[TESTE 1] Resolvendo DNS...");
    IPAddress resolvedIP;
    if (WiFi.hostByName("monitoramentoforno.com.br", resolvedIP))
    {
        Serial.print("OK - IP resolvido: ");
        Serial.println(resolvedIP);
    }
    else
    {
        Serial.println("FALHA - DNS nao resolveu o dominio");
    }

    // ---------- TESTE 2: HTTP puro (porta 80) ----------
    Serial.println("\n[TESTE 2] HTTP puro na porta 80...");
    {
        HTTPClient http;
        WiFiClient client;
        http.begin(client, "http://monitoramentoforno.com.br/v1/fornos/auth");
        http.addHeader("Content-Type", "application/json");
        int code = http.POST("{}");
        Serial.printf("Codigo: %d | Mensagem: %s\n", code, http.errorToString(code).c_str());
        if (code > 0)
        {
            Serial.println("Resposta do servidor: " + http.getString());
        }
        http.end();
    }

    delay(1000);

    // ---------- TESTE 3: HTTPS com setInsecure (porta 443) ----------
    Serial.println("\n[TESTE 3] HTTPS com setInsecure() na porta 443...");
    {
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();
        http.begin(client, "https://monitoramentoforno.com.br/v1/fornos/auth");
        http.addHeader("Content-Type", "application/json");
        int code = http.POST("{}");
        Serial.printf("Codigo: %d | Mensagem: %s\n", code, http.errorToString(code).c_str());
        if (code > 0)
        {
            Serial.println("Resposta do servidor: " + http.getString());
        }
        http.end();
    }

    delay(1000);

    // ---------- TESTE 4: site externo conhecido (controle - testa se HTTPS funciona em geral) ----------
    Serial.println("\n[TESTE 4] HTTPS para site externo (google.com) - teste de controle...");
    {
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();
        http.begin(client, "https://www.google.com");
        int code = http.GET();
        Serial.printf("Codigo: %d | Mensagem: %s\n", code, http.errorToString(code).c_str());
        http.end();
    }

    Serial.println("\n===== FIM DO DIAGNOSTICO =====\n");
}

#endif

// ==========================
// LOCK TLS (só uma conexão TLS por vez, incluindo o handshake do WebSocket)
// ==========================

struct TlsLock
{
    bool ativo = false;
    TlsLock()
    {
        // mutexHttps ainda é NULL durante o setup(); nesse caso não trava
        if (mutexHttps != nullptr)
            ativo = (xSemaphoreTake(mutexHttps, pdMS_TO_TICKS(15000)) == pdTRUE); // nunca trava para sempre
    }
    ~TlsLock()
    {
        if (ativo)
            xSemaphoreGive(mutexHttps);
    }
};

static void logarMemoria(const char *onde)
{
    // getMaxAllocHeap = MAIOR bloco contíguo (é o que o TLS precisa)
    Serial.printf("[MEM] %s | livre=%u | maiorBloco=%u | minimoHistorico=%u\n",
                  onde, ESP.getFreeHeap(), ESP.getMaxAllocHeap(), ESP.getMinFreeHeap());
}

// ==========================
// INICIAR HTTP
// ==========================

static bool iniciarHttp(HTTPClient &http, WiFiClientSecure &client, const String &url)
{
#ifdef MODO_INSEGURO
    client.setInsecure();
#else
    client.setCACert(ROOT_CA);
#endif
    http.setConnectTimeout(8000);
    http.setTimeout(8000);
    http.setReuse(false);
    return http.begin(client, url);
}

// ==========================
// AGUARDAR HORÁRIO
// ==========================

static bool aguardarHora(unsigned long timeoutMs)
{

    unsigned long inicio = millis();

    while (time(nullptr) < 1700000000)
    {
        if (millis() - inicio >= timeoutMs)
        {
            Serial.println("Relogio nao sincronizou a tempo.");
            return false;
        }
        delay(250);
    }
    return true;
}

// ==========================
// LOGIN
// ==========================

void fazerLogin()
{
    String novoToken = "";

    { // escopo: lock é liberado só depois de client/http serem destruídos
        TlsLock lock;
        WiFiClientSecure client;
        HTTPClient http;

        String urlAuth = String(API_BASE_URL) + "/v1/fornos/auth";
        Serial.print("Tentando fazer login na URL: ");
        Serial.println(urlAuth);

        if (!iniciarHttp(http, client, urlAuth))
        {
            Serial.println("Falha ao iniciar conexao HTTPS.");
            return;
        }

        http.addHeader("Content-Type", "application/json");

        String secretLimpa = condicoes.deviceSecret;
        secretLimpa.trim();
        if (secretLimpa.length() > 36)
        {
            secretLimpa = secretLimpa.substring(0, 36);
        }

        JsonDocument doc;
        doc["secret"] = secretLimpa;
        doc["serialNumber"] = condicoes.serialNumber;

        String jsonOutput;
        serializeJson(doc, jsonOutput);

        Serial.println("Enviando JSON");

        int code = http.POST(jsonOutput);

        if (code == 200)
        {
            JsonDocument res;
            deserializeJson(res, http.getString());
            const char *t = res["token"] | "";
            novoToken = String(t);
            Serial.println("Login com sucesso!");
        }
        else
        {
            char erroTls[100] = "";
            client.lastError(erroTls, sizeof(erroTls));
            Serial.printf("ERRO LOGIN: codigo=%d msg=%s | TLS=%s\n", code, http.errorToString(code).c_str(), erroTls);
            logarMemoria("falha login");
        }

        http.end();
    }

    if (!novoToken.isEmpty())
    {
        setTokenJWT(novoToken);

        xSemaphoreTake(mutexLoginWebSocket, portMAX_DELAY);
        condicoes.tokenUsuario = novoToken;
        condicoes.fezLogin = true;
        xSemaphoreGive(mutexLoginWebSocket);
    }
}

// ==========================
// GARANTIA LOGIN
// ==========================

bool garantirLogin()
{
    ;

    if (!getTokenJWT().isEmpty())
    {
        return true;
    }

    fazerLogin();

    return !getTokenJWT().isEmpty();
}

// ==========================
// REQUEST CENTRAL
// ==========================

int enviarRequisicaoHTTP(
    const String &url,
    const String &metodo,
    const String &payload,
    String *response)
{
    if (!garantirLogin())
        return -1;

    const int MAX_TENTATIVAS = 3;
    int codigo = -2;

    for (int tentativa = 1; tentativa <= MAX_TENTATIVAS; tentativa++)
    {
        bool refazerLogin = false;
        bool repetir = false;

        { // <-- escopo: client e http são destruídos antes do lock ser liberado
            TlsLock lock;
            if (ESP.getMaxAllocHeap() < 45000)
                logarMemoria("heap baixo antes do HTTPS");

            WiFiClientSecure secureClient;
            HTTPClient http;

            if (!iniciarHttp(http, secureClient, url))
            {
                codigo = -3;
                repetir = true;
            }
            else
            {
                http.addHeader("Content-Type", "application/json");
                http.addHeader("Authorization", "Bearer " + getTokenJWT());

                if (metodo == "POST")      codigo = http.POST(payload);
                else if (metodo == "PUT")  codigo = http.PUT(payload);
                else if (metodo == "GET")  codigo = http.GET();
                else                       codigo = -2;

                Serial.printf("HTTP retornou: %d (tentativa %d/%d)\n", codigo, tentativa, MAX_TENTATIVAS);

                if (codigo < 0)
                {
                    char erroTls[100];
                    secureClient.lastError(erroTls, sizeof(erroTls));
                    Serial.printf("[TLS] %s | %s\n", erroTls, http.errorToString(codigo).c_str());
                    logarMemoria("falha HTTPS");
                    repetir = (codigo != -2); // -2 = método inválido, não adianta repetir
                }
                else if (codigo == 401)
                {
                    refazerLogin = true;
                }
                else if (response != nullptr)
                {
                    *response = http.getString(); // 2xx, 404, 5xx...
                }
            }
            http.end();
        } // <-- aqui a memória TLS já foi devolvida e o lock liberado

        if (refazerLogin)
        {
            setTokenJWT("");
            fazerLogin();
            continue;
        }

        if (!repetir)
            return codigo;

        vTaskDelay(pdMS_TO_TICKS(1000 * tentativa)); // espera 1s, 2s, 3s
    }

    return codigo;
}

// ==========================
// SESSÃO
// ==========================

bool iniciarSessao()
{

    if (!getSessaoId().isEmpty())
        return true;

    String resposta;
    
    Serial.print("Heap livre antes de iniciar sessão: ");
    Serial.println(ESP.getFreeHeap());

    // CORREÇÃO: Enviando payload JSON vazio "{}" em vez de string vazia ""
    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/iniciar",
        "POST",
        "{}",
        &resposta);

    Serial.print("Código recebido em iniciarSessao: ");
    Serial.println(code);
    Serial.print("Resposta bruta da API: ");
    Serial.println(resposta);

    if (code == 201)
    {

        JsonDocument doc;

        DeserializationError erro = deserializeJson(doc, resposta);

        if (erro)
        {
            Serial.println(erro.c_str());
            return false;
        }

        setSessaoId(doc["id"].as<String>());

        Serial.println("Sessão criada:");
        Serial.println(getSessaoId());

        return true;
    }

    Serial.println("Falha ao criar sessão");
    return false;
}

// ==========================
// ENCERRAR SESSÃO
// ==========================

void encerrarSessao()
{
    if (getSessaoId().isEmpty())
        return;

    String body;
    JsonDocument doc;

    doc["estadoSistemaFinal"] = obterEstadoSistemaTexto(condicoes.estadoAtual);
    doc["estadoFornoFinal"] = obterEstadoFornoTexto(condicoes.estadoFornoAtual);

    serializeJson(doc, body);

    String resposta;

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/" + getSessaoId() + "/encerrar",
        "PUT",
        body,
        nullptr);

    Serial.printf("Encerrar sessão: %d\n", code);

    setSessaoId("");
}

// ==========================
// ATUALIZAR SESSÃO
// ==========================

void atualizarSessao()
{

    String body;
    JsonDocument doc;

    doc["estadoSistemaAtual"] = obterEstadoSistemaTexto(condicoes.estadoAtual);
    doc["estadoFornoAtual"] = obterEstadoFornoTexto(condicoes.estadoFornoAtual);

    serializeJson(doc, body);

    String resposta;

    if (getSessaoId().isEmpty())
    {
        return; 
    }

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/" + getSessaoId() + "/atualizar",
        "PUT",
        body,
        nullptr);

    Serial.printf("Atualizar sessão: %d\n", code);
}

// ==========================
// ENVIAR TELEMETRIA
// ==========================

void enviarTelemetria(double temperaturaAtual, double temperaturaUltima, const String &estadoSistema, const String &estadoForno, int tempoLigado)
{

    if (getSessaoId().isEmpty())
    {
        if (estadoForno == "FORNO_DESLIGADO")
        {
            
            return;
        }
        
        // Tenta iniciar apenas se necessário
        Serial.println("[TELEM] sem sessao, tentando iniciar");
        if (!iniciarSessao())
        {
            Serial.println("Nao foi possivel iniciar a sessao.");
            return;
        }
    }

    JsonDocument doc;
    String body;

    // CORREÇÃO (opcional mas garante envio): removido a trava de <= 0 se o sensor puder ler 0, mas mantido o isnan
    if (isnan(temperaturaAtual))
    {
        return; // Não envia lixo para o backend se o sensor falhou
    }

    doc["temperaturaAtual"] = temperaturaAtual;
    doc["temperaturaUltima"] = isnan(temperaturaUltima) ? 0.0 : temperaturaUltima;
    doc["estadoSistema"] = estadoSistema;
    doc["estadoForno"] = estadoForno;
    doc["tempoLigadoMinutos"] = tempoLigado;

    serializeJson(doc, body);

    String resposta;

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/telemetrias",
        "POST",
        body,
        &resposta);

    Serial.printf("Código: %d\n", code);
    Serial.println("Resposta:");
    Serial.println(resposta);
}

// ==========================
// TEMPERATURA
// ==========================

void enviarTemperatura(const String &estadoFornoTexto, double temperaturaAtual, double temperaturaUltima)
{
    Serial.printf("[TEMP] chamada: forno=%s atual=%.2f sessao='%s'\n",
                  estadoFornoTexto.c_str(), temperaturaAtual, getSessaoId().c_str());

    if (estadoFornoTexto == "FORNO_DESLIGADO")
    {
        Serial.println("[TEMP] saiu: forno desligado");
        if (!getSessaoId().isEmpty())
        {
            setSessaoId("");
        }
        return;
    }

    if (isnan(temperaturaAtual))
    {
        Serial.println("[TEMP] saiu: temperatura invalida");
        return;
    }

    if (getSessaoId().isEmpty())
    {
        if (!iniciarSessao())
        {
            Serial.println("[TEMP] saiu: nao foi possivel iniciar a sessao");
            return;
        }
    }

    JsonDocument doc;
    String body;

    doc["sessaoId"] = getSessaoId();
    doc["temperaturaAtual"] = temperaturaAtual;
    doc["temperaturaUltima"] = temperaturaUltima;

    serializeJson(doc, body);

    String resposta;

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/temperaturas",
        "POST",
        body,
        &resposta);

    Serial.printf("[TEMP] Codigo envio temperatura: %d | resposta: %s\n", code, resposta.c_str());

    if (code == 401 || code == 403 || code == 404)
    {
        Serial.println("[TEMP] Sessao invalida no servidor. Limpando ID local...");
        setSessaoId("");
    }
}

// ==========================
// EVENTOS
// ==========================

bool enviarEvento(String tipo)
{
    if (!iniciarSessao() || getSessaoId().isEmpty())
    {
        Serial.println("Falha ao enviar evento: Sessao não iniciada ou sessaoId vazio.");
        return false; // taskNuvem devolve o evento à fila
    }

    JsonDocument doc;
    String body;
    body.reserve(128);

    doc["sessaoId"] = getSessaoId();
    doc["tipo"] = tipo;

    serializeJson(doc, body);

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/eventos",
        "POST",
        body,
        nullptr);

    Serial.printf("Evento HTTP Código: %d\n", code);

    if (code >= 200 && code < 300)
        return true;

    if (code == 404)
    {
        setSessaoId(""); // sessão inválida: próxima tentativa cria outra
        return false;
    }

    if (code < 0 || code >= 500 || code == 401)
        return false; // falha temporária: tenta de novo

    return true; // outro 4xx: reenviar não adianta, descarta
}

void verificarEstadoDispositivo()
{

    conectarWiFi();

    if (!aguardarHora(15000))
    {
        Serial.println("Seguindo sem hora válida! HTTPS pode falhar.");
    }

    if (preferences.getString("secret", "").isEmpty())
    {
        condicoes.espConfigurado = false;

        JsonDocument doc;
        HTTPClient http;
        WiFiClientSecure client;

        if (!iniciarHttp(http, client, String(API_BASE_URL) + "/v1/fornos/auto-provisionar"))
        {
            return;
        }

        http.addHeader("Content-Type", "application/json");

        doc["serialNumber"] = condicoes.serialNumber;

        String jsonOutput;
        serializeJson(doc, jsonOutput);

        Serial.println("Enviando JSON");

        int code = http.POST(jsonOutput);

        if (code == 200)
        {

            String payload = http.getString(); // Pega o corpo da resposta

            JsonDocument resDoc; // <-- NOME ALTERADO AQUI
            deserializeJson(resDoc, payload);

            String secretRecebida = resDoc["secret"].as<String>(); // Extrai a secret do JSON
            preferences.putString("secret", secretRecebida);       // Salva na memória não-volátil

            condicoes.deviceSecret = secretRecebida; // Atualiza variável global com a nova secret
            condicoes.espConfigurado = true;

            Serial.println("Auto-provisionamento concluído com sucesso!");

            solicitarDesligamentoBluetooth();
            fazerLogin();
        }
        else
        {
            Serial.printf("Falha no provisionamento. Código HTTP: %d\n", code);

            Serial.println("Ativando Bluetooth para diagnóstico e aguardando...");
            // Movemos o log para cá, onde ele faz sentido:
            Serial.println("Aguardando configuração Bluetooth...");
            inicializarBluetooth();
        }

        http.end();
    }
    else
    {
        condicoes.espConfigurado = true;
        Serial.println("Dispositivo já provisionado. Iniciando rotina normal...");
#ifdef MODO_DEBUG
        diagnosticoCompleto();
#endif
        solicitarDesligamentoBluetooth();
        fazerLogin();
    }
}