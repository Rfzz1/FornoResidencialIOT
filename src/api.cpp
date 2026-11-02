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
    return sessaoId;
}

void setSessaoId(String SessaoId)
{
    sessaoId = SessaoId;
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

            if (evento != NENHUM)
            {

                enviarEvento(obterEventoSistemaTexto(evento));
            }
        }

        while (xQueueReceive(eventosFornoQueue, &eventoForno, 0) == pdTRUE)
        {

            if (estadoFornoAnterior == FORNO_DESLIGADO && eventoForno != FORNO_DESLIGADO)
            {
                iniciarSessao();
            }

            if (eventoForno == FORNO_DESLIGADO && estadoFornoAnterior == FORNO_ESFRIANDO)
            {
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
// INICIAR HTTP
// ==========================

static bool iniciarHttp(HTTPClient &http, WiFiClientSecure &client, const String &url)
{

    client.setInsecure();

    http.setTimeout(5000);
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
    JsonDocument doc;
    HTTPClient http;
    WiFiClientSecure client;

    if (!iniciarHttp(http, client, String(API_BASE_URL) + "/v1/fornos/auth"))
    {
        Serial.println("Falha ao iniciar conexao HTTPS.");
        return;
    }

    http.addHeader("Content-Type", "application/json");

    // Limpeza da secret (aqui está o corte dos 36 caracteres para garantir)
    String secretLimpa = condicoes.deviceSecret;
    secretLimpa.trim();
    if (secretLimpa.length() > 36)
    {
        secretLimpa = secretLimpa.substring(0, 36);
    }
    doc["secret"] = secretLimpa;
    doc["serialNumber"] = condicoes.serialNumber;

    String jsonOutput;

    serializeJson(doc, jsonOutput);

    Serial.println("Enviando JSON");

    int code = http.POST(jsonOutput);

    if (code == 200)
    {
        String payload = http.getString();
        JsonDocument res;
        deserializeJson(res, payload);

        xSemaphoreTake(mutexLoginWebSocket, portMAX_DELAY);
        tokenJWT = res["token"].as<String>();
        condicoes.tokenUsuario = tokenJWT;
        condicoes.fezLogin = true;
        xSemaphoreGive(mutexLoginWebSocket);

        Serial.println("Login com sucesso!");
    }
    else
    {
        Serial.printf("DEBUG ERRO HTTP: codigo=%d mensagem=%s\n", code, http.errorToString(code).c_str());
    }

    http.end();
}

// ==========================
// GARANTIA LOGIN
// ==========================

bool garantirLogin()
{
    ;

    if (!tokenJWT.isEmpty())
    {
        return true;
    }

    fazerLogin();

    return !tokenJWT.isEmpty();
}

// ==========================
// REQUEST CENTRAL
// ==========================

int enviarRequisicaoHTTP(
    const String &url,
    const String &metodo,
    const String &payload,
    String *response = nullptr)
{

    // controle de recuperação de sessão
    int tentativas = 0;
    const int MAX_TENTATIVAS = 3;
    bool sucesso = false;
    int codigo = -2;

    while (tentativas < MAX_TENTATIVAS && !sucesso)
    {

        xSemaphoreTake(mutexHttps, portMAX_DELAY);

        WiFiClientSecure secureClient;
        HTTPClient http;

        Serial.print("URL: ");
        Serial.println(url);

        if (!iniciarHttp(http, secureClient, url))
        {
            xSemaphoreGive(mutexHttps);
            return -3; // Erro de conexão
        }

        if (!garantirLogin())
        {
            http.end();
            xSemaphoreGive(mutexHttps);
            return -1;
        }

        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", "Bearer " + tokenJWT);

        Serial.print("Payload: ");
        Serial.println(payload);

        Serial.print("Metodo: ");
        Serial.println(metodo);

        if (metodo == "POST")
            codigo = http.POST(payload);
        else if (metodo == "PUT")
            codigo = http.PUT(payload);
        else if (metodo == "GET")
            codigo = http.GET();
        else
        {
            http.end();
            xSemaphoreGive(mutexHttps);
            return -2;
        }

        Serial.printf("HTTP retornou: %d\n", codigo);

        if (codigo >= 200 && codigo < 300) // Sucesso
        {
            if (response != nullptr)
            {
                *response = http.getString();
            }
            sucesso = true;
        }
        else if (codigo == 401)
        {

            // =========================
            // TOKEN INVÁLIDO
            // =========================

            xSemaphoreTake(mutexLoginWebSocket, portMAX_DELAY);
            tokenJWT = "";
            condicoes.tokenUsuario = "";
            condicoes.fezLogin = false;
            xSemaphoreGive(mutexLoginWebSocket);
            tentativas++;
        }
        else if (codigo == 404)
        {
            // =========================
            // SESSÃO INVÁLIDA
            // =========================

            sessaoId = "";

            if (iniciarSessao())
            {
                tentativas++;
            }
            else
            {
                break;
            }
        }
        else
        {
            if (response != nullptr)
            {
                *response = http.getString();
            }
            sucesso = true;
        }
        http.end();
        xSemaphoreGive(mutexHttps);
    }

    return codigo;
}

// ==========================
// SESSÃO
// ==========================

bool iniciarSessao()
{

    if (!sessaoId.isEmpty())
        return true;

    String resposta;

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/iniciar",
        "POST",
        "",
        &resposta);

    Serial.print("Código recebido em iniciarSessao: ");
    Serial.println(code);

    if (code == 201)
    {

        JsonDocument doc;

        DeserializationError erro = deserializeJson(doc, resposta);

        if (erro)
        {
            Serial.println(erro.c_str());
            return false;
        }

        sessaoId = doc["id"].as<String>();

        Serial.println("Sessão criada:");
        Serial.println(sessaoId);

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

    String body;
    JsonDocument doc;

    doc["estadoSistemaFinal"] = obterEstadoSistemaTexto(condicoes.estadoAtual);
    doc["estadoFornoFinal"] = obterEstadoFornoTexto(condicoes.estadoFornoAtual);

    serializeJson(doc, body);

    String resposta;

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/" + sessaoId + "/encerrar",
        "PUT",
        body,
        &resposta);

    Serial.printf("Encerrar sessão: %d\n", code);
    Serial.println("Resposta:");
    Serial.println(resposta);

    sessaoId = "";
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

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/" + sessaoId + "/atualizar",
        "PUT",
        body,
        &resposta);

    Serial.printf("Atualizar sessão: %d\n", code);
    Serial.println("Resposta:");
    Serial.println(resposta);
}

// ==========================
// ENVIAR TELEMETRIA
// ==========================

    void enviarTelemetria(double temperaturaAtual, double temperaturaUltima, const String &estadoSistema, const String &estadoForno, int tempoLigado)
    {

        if (sessaoId.isEmpty())
        {
            if (estadoForno == "FORNO_DESLIGADO")
            {
                return;
            }
            // Se estiver ligado mas perdeu a sessão (ex: reiniciou), tenta recuperar
            if (!iniciarSessao())
            {
                Serial.println("Nao foi possivel iniciar a sessao.");
                return;
            }
        }

        JsonDocument doc;
        String body;

        if(isnan(temperaturaAtual)) {
            doc["temperaturaAtual"] = isnan(temperaturaUltima) ? 0.0 : temperaturaUltima;
        } else {
            doc["temperaturaAtual"] = temperaturaAtual;
        }

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

    if (sessaoId.isEmpty())
    {
        if (estadoFornoTexto == "FORNO_DESLIGADO")
        {
            return;
        }
        if (!iniciarSessao())
        {
            Serial.println("Nao foi possivel iniciar a sessao.");
            return;
        }
    }

    Serial.printf(
        "Heap = %u\n",
        ESP.getFreeHeap());

    JsonDocument doc;
    String body;

    Serial.print("Sessao: ");
    Serial.println(sessaoId);

    if (!iniciarSessao())
    {
        Serial.println("Nao foi possivel iniciar a sessao.");
        return;
    }

    Serial.print("Sessao: ");
    Serial.println(sessaoId);

    doc["sessaoId"] = sessaoId;
    doc["temperaturaAtual"] = temperaturaAtual;
    doc["temperaturaUltima"] = temperaturaUltima;

    serializeJson(doc, body);

    String resposta;

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/temperaturas",
        "POST",
        body,
        &resposta);

    Serial.printf("Código: %d\n", code);
    Serial.println("Resposta:");
    Serial.println(resposta);

    Serial.printf("Envio temperatura: %d\n", code);
}

// ==========================
// EVENTOS
// ==========================

void enviarEvento(String tipo)
{

    if (!iniciarSessao() || sessaoId.isEmpty())
    {
        Serial.println("Falha ao enviar evento: Sessao não iniciada ou sessaoId vazio.");
        return;
    }

    JsonDocument doc;
    String body;
    body.reserve(128);

    doc["sessaoId"] = sessaoId;
    doc["tipo"] = tipo;

    serializeJson(doc, body);

    int code = enviarRequisicaoHTTP(
        URL_EVENTOS,
        "POST",
        body,
        nullptr);

    Serial.printf("Evento HTTP Código: %d\n", code);
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
