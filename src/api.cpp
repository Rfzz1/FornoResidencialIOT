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
Preferences preferences;

double temperaturaAtual = 0;
double temperaturaUltima = 0;

// controle de recuperação de sessão
static bool tentandoRecuperarSessao = false;
static int tentativasSessao = 0;

//Task para enviar telemetria periodicamente
void taskNuvem(void *parameter) {

    double temperaturaAtualQueue;
    double temperaturaUltimaQueue;
    String estadoSistemaTexto = "";
    String estadoFornoTexto = "";
    uint32_t tempoLigadoMin = 0;

    eventoSistema evento;
    estadoForno eventoForno;

    for (;;) {
        
        xSemaphoreTake(mutexTelemetria, portMAX_DELAY);
            temperaturaAtualQueue = dados.TEMP_ATUAL;
            temperaturaUltimaQueue = dados.ULTIMA_TEMP;
            estadoSistemaTexto = obterEstadoSistemaTexto(dados.estadoAtual);
            estadoFornoTexto = obterEstadoFornoTexto(dados.estadoFornoAtual);
            tempoLigadoMin = dados.tempoLigadoMinutos;
        xSemaphoreGive(mutexTelemetria);

        enviarTelemetria(temperaturaAtualQueue, temperaturaUltimaQueue, estadoSistemaTexto, estadoFornoTexto, tempoLigadoMin);

        enviarTemperatura(estadoFornoTexto, temperaturaAtualQueue, temperaturaUltimaQueue);

        if (xQueueReceive(eventosQueue, &evento, pdMS_TO_TICKS(1000)) == pdTRUE) {
            enviarEvento(obterEventoSistemaTexto(evento));
        }

        if (xQueueReceive(eventosFornoQueue, &eventoForno, pdMS_TO_TICKS(1000)) == pdTRUE) {
            enviarEvento(obterEstadoFornoTexto(eventoForno));
            
            if (eventoForno == FORNO_DESLIGADO) {
                encerrarSessao();
            }
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);

    }
}


void diagnosticoCompleto() {

    Serial.printf("Heap livre: %d bytes\n", ESP.getFreeHeap());
 
    Serial.println("\n===== INICIANDO DIAGNOSTICO =====");
 
    // ---------- TESTE 1: DNS ----------
    Serial.println("\n[TESTE 1] Resolvendo DNS...");
    IPAddress resolvedIP;
    if (WiFi.hostByName("monitoramentoforno.com.br", resolvedIP)) {
        Serial.print("OK - IP resolvido: ");
        Serial.println(resolvedIP);
    } else {
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
        if (code > 0) {
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
        if (code > 0) {
            Serial.println("Resposta do servidor: " + http.getString());
        }
        http.end();
    }
 
    delay(1000);
 
    // ---------- TESTE 4: HTTPS direto no IP (porta 443), sem domínio ----------
    Serial.println("\n[TESTE 4] HTTPS direto no IP 54.207.91.135...");
    {
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();
        http.begin(client, "https://54.207.91.135/v1/fornos/auth");
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Host", "monitoramentoforno.com.br"); // necessário para SNI/virtual host
        int code = http.POST("{}");
        Serial.printf("Codigo: %d | Mensagem: %s\n", code, http.errorToString(code).c_str());
        if (code > 0) {
            Serial.println("Resposta do servidor: " + http.getString());
        }
        http.end();
    }
 
    delay(1000);
 
    // ---------- TESTE 5: site externo conhecido (controle - testa se HTTPS funciona em geral) ----------
    Serial.println("\n[TESTE 5] HTTPS para site externo (google.com) - teste de controle...");
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

// ==========================
// LOGIN
// ==========================

void fazerLogin() {
    JsonDocument doc;
    HTTPClient http;
    WiFiClient client;

    IPAddress resolvedIP;
    if (WiFi.hostByName("monitoramentoforno.com.br", resolvedIP)) {
        Serial.print("IP resolvido: ");
        Serial.println(resolvedIP);
    } else {
        Serial.println("Falha ao resolver DNS!");
    }
    
    http.begin(client, String(API_BASE_URL) + "/v1/fornos/auth"); 
    http.addHeader("Content-Type", "application/json");

    // Limpeza da secret (aqui está o corte dos 36 caracteres para garantir)
    String secretLimpa = dados.deviceSecret;
    secretLimpa.trim(); 
    if (secretLimpa.length() > 36) {
        secretLimpa = secretLimpa.substring(0, 36);
    }
    doc["secret"] = secretLimpa;
    doc["serialNumber"] = dados.serialNumber;

    String jsonOutput;

    serializeJson(doc, jsonOutput);

    Serial.println("Enviando JSON: " + jsonOutput);

    int code = http.POST(jsonOutput);

    if (code == 200) {
        String payload = http.getString();
        JsonDocument res;
        deserializeJson(res, payload);
        tokenJWT = res["token"].as<String>(); // Certifique-se de salvar o token!
        Serial.println("Login com sucesso!");
    } else {
        Serial.printf("DEBUG ERRO HTTP: codigo=%d mensagem=%s\n", code, http.errorToString(code).c_str());
    }

    http.end();
}

void salvarSecretBluetooth(String recebidoDoTerminal) {
    String secretLimpa = "";
    for (char c : recebidoDoTerminal) {
        if (isAlphaNumeric(c) || c == '-') { 
            secretLimpa += c;
        }
    }
    // Salva a versão limpa nas Preferences
    preferences.putString("secret", secretLimpa); 
    dados.deviceSecret = secretLimpa;
    Serial.println("Nova secret salva com sucesso!");
}

// ==========================
// GARANTIA LOGIN
// ==========================

bool garantirLogin() {

    Serial.println("GL 1");

    if (!tokenJWT.isEmpty()) {
        Serial.println("GL 2");
        return true;
    }

    Serial.println("GL 3");

    fazerLogin();

    Serial.println("GL 4");

    Serial.print("Token tamanho: ");
    Serial.println(tokenJWT.length());

    return !tokenJWT.isEmpty();
}

// ==========================
// REQUEST CENTRAL
// ==========================

int enviarRequisicaoHTTP(
    const String &url,
    const String &metodo,
    const String &payload,
    String *response = nullptr,
    int tentativa = 0
) {

    WiFiClient client;
    WiFiClientSecure secureClient;

    HTTPClient http;

    Serial.print("URL: ");
    Serial.println(url);

    if (url.startsWith("https://")) {
        Serial.println("HTTPS");
        secureClient.setInsecure();
        http.begin(secureClient, url);
    } else {
        Serial.println("HTTP");
        http.begin(client, url);
    }

    http.setTimeout(5000);     // espera no máximo 5 segundos
    http.setReuse(false);       // fecha a conexão após cada requisição

    Serial.println("BEGIN OK");

    Serial.print("Token tamanho: ");
    Serial.println(tokenJWT.length());

    Serial.println("3");

    bool ok = garantirLogin(); 

    Serial.print("garantirLogin = ");
    Serial.println(ok);

    if (!ok) {
        Serial.println("SAIU -1");
        http.end();
        return -1;
    }

    Serial.println("ADD HEADER");

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + tokenJWT);

    int codigo;

    Serial.println("4");

    Serial.print("Payload: ");
    Serial.println(payload);

    Serial.print("Metodo: ");
    Serial.println(metodo);
    
    if (metodo == "POST") codigo = http.POST(payload);
    else if (metodo == "PUT") codigo = http.PUT(payload);
    else if (metodo == "GET") codigo = http.GET();
    else {
        http.end();
        return -2;
    }

    Serial.printf("HTTP retornou: %d\n", codigo);

    String respostaHttp;

    Serial.println("5");

    if (codigo > 0) {

        Serial.println("6");

        respostaHttp = http.getString();
        Serial.println("Resposta da API:");
        Serial.println(respostaHttp);

        Serial.println("7");
    } else {
        Serial.println("8");
        Serial.println(http.errorToString(codigo));
    }

    if (response != nullptr) {
        *response = respostaHttp;
    }
    http.end();
    Serial.println("9");

    // =========================
    // TOKEN INVÁLIDO
    // =========================

    if (codigo == 401) {
        
        if(tentativa >= 3) {
            Serial.println("Falha crítica no login.");
            return codigo;
        }

        tokenJWT = "";

        if (!garantirLogin())
            return codigo;

        return enviarRequisicaoHTTP(url, metodo, payload, response, tentativa + 1);
    }

    // =========================
    // SESSÃO INVÁLIDA
    // =========================

    if (codigo == 404) {

        if (tentativasSessao >= 3) {
            Serial.println("Falha crítica na sessão.");
            return codigo;
        }

        if (!tentandoRecuperarSessao) {

            tentandoRecuperarSessao = true;
            tentativasSessao++;

            Serial.println("Recriando sessão...");

            sessaoId = "";

            if (iniciarSessao()) {
                tentativasSessao = 0;
                tentandoRecuperarSessao = false;
                return enviarRequisicaoHTTP(url, metodo, payload, response);
            }

            tentandoRecuperarSessao = false;
        }
    }

    return codigo;
}

// ==========================
// SESSÃO
// ==========================

bool iniciarSessao() {

    if (!sessaoId.isEmpty())
        return true;

    String resposta;

    Serial.println("Token:");
    Serial.println(tokenJWT);

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/iniciar",
        "POST",
        "",
        &resposta
    );

    Serial.print("Código recebido em iniciarSessao: ");
    Serial.println(code);

if (code == 201) {

    JsonDocument doc;

    DeserializationError erro = deserializeJson(doc, resposta);

    if (erro) {
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

void encerrarSessao() {

    String body;
    JsonDocument doc;

    doc["estadoSistemaFinal"] = obterEstadoSistemaTexto(dados.estadoAtual);
    doc["estadoFornoFinal"] = obterEstadoFornoTexto(dados.estadoFornoAtual);

    serializeJson(doc, body);

    String resposta;

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/" + sessaoId + "/encerrar",
        "PUT",
        body,
        &resposta
    );

    Serial.printf("Encerrar sessão: %d\n", code);
    Serial.println("Resposta:");
    Serial.println(resposta);

    sessaoId = "";
}

// ==========================
// ATUALIZAR SESSÃO
// ==========================

void atualizarSessao() {

    String body;
    JsonDocument doc;

    doc["estadoSistemaAtual"] = obterEstadoSistemaTexto(dados.estadoAtual);
    doc["estadoFornoAtual"] = obterEstadoFornoTexto(dados.estadoFornoAtual);

    serializeJson(doc, body);

    String resposta;

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/" + sessaoId + "/atualizar",
        "PUT",
        body,
        &resposta
    );

    Serial.printf("Atualizar sessão: %d\n", code);
    Serial.println("Resposta:");
    Serial.println(resposta);
}

// ==========================
// ENVIAR TELEMETRIA
// ==========================

void enviarTelemetria(double temperaturaAtual, double temperaturaUltima, const String& estadoSistema, const String& estadoForno, int tempoLigado) {

    if (sessaoId.isEmpty()) {
        if (estadoForno == "FORNO_DESLIGADO") {
            return; 
        }
        // Se estiver ligado mas perdeu a sessão (ex: reiniciou), tenta recuperar
        if (!iniciarSessao()) {
            Serial.println("Nao foi possivel iniciar a sessao.");
            return;
        }
    }

    JsonDocument doc;
    String body;

    if (!iniciarSessao()) {
        Serial.println("Nao foi possivel iniciar a sessao.");
        return;
    }

    doc["temperaturaAtual"] = temperaturaAtual;
    doc["temperaturaUltima"] = temperaturaUltima;
    doc["estadoSistema"] = estadoSistema;
    doc["estadoForno"] = estadoForno;
    doc["tempoLigadoMinutos"] = tempoLigado;


    serializeJson(doc, body);

    String resposta;

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/telemetrias",
        "POST",
        body,
        &resposta
    );

    Serial.printf("Código: %d\n", code);
    Serial.println("Resposta:");
    Serial.println(resposta);
}

// ==========================
// TEMPERATURA
// ==========================

void enviarTemperatura(const String& estadoFornoTexto, double temperaturaAtual, double temperaturaUltima) {

    if (sessaoId.isEmpty()) {
        if (estadoFornoTexto == "FORNO_DESLIGADO") {
            return; 
        }
        if (!iniciarSessao()) {
            Serial.println("Nao foi possivel iniciar a sessao.");
            return;
        }
    }

    Serial.printf(
    "Heap = %u\n",
    ESP.getFreeHeap()
);

    JsonDocument doc;
    String body;

    Serial.print("Sessao: ");
    Serial.println(sessaoId);

    if (!iniciarSessao()) {
    Serial.println("Nao foi possivel iniciar a sessao.");
    return;
}

Serial.print("Token: ");
Serial.println(tokenJWT.length());

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
        &resposta
    );

    Serial.printf("Código: %d\n", code);
    Serial.println("Resposta:");
    Serial.println(resposta);

    Serial.printf("Envio temperatura: %d\n", code);
}

// ==========================
// EVENTOS
// ==========================

void enviarEvento(String tipo) {

    if (!iniciarSessao() || sessaoId.isEmpty()) {
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
        nullptr
    );

    Serial.printf("Evento HTTP Código: %d\n", code);
}

void verificarEstadoDispositivo() {

    conectarWiFi();

    if (preferences.getString("secret", "").isEmpty()) {
        dados.espConfigurado = false;

        JsonDocument doc;
        HTTPClient http;
        WiFiClient client;
        
        http.begin(client, String(API_BASE_URL) + "/v1/fornos/auto-provisionar"); 
        http.addHeader("Content-Type", "application/json");

        doc["serialNumber"] = dados.serialNumber;

        String jsonOutput;
        serializeJson(doc, jsonOutput);

        Serial.println("Enviando JSON: " + jsonOutput);

        int code = http.POST(jsonOutput);

        if (code == 200) {
            
            String payload = http.getString(); // Pega o corpo da resposta

            JsonDocument resDoc; // <-- NOME ALTERADO AQUI
            deserializeJson(resDoc, payload);
            
            String secretRecebida = resDoc["secret"].as<String>(); // Extrai a secret do JSON
            preferences.putString("secret", secretRecebida); // Salva na memória não-volátil

            dados.deviceSecret = secretRecebida; // Atualiza variável global com a nova secret
            dados.espConfigurado = true;

            Serial.println("Auto-provisionamento concluído com sucesso!");

            fazerLogin();

        } else {
            Serial.printf("Falha no provisionamento. Código HTTP: %d\n", code);

            Serial.println("Ativando Bluetooth para diagnóstico e aguardando...");
            // Movemos o log para cá, onde ele faz sentido:
            Serial.println("Aguardando configuração Bluetooth...");
            inicializarBluetooth();
        }

        http.end();
    
    } else {
        dados.espConfigurado = true;
        Serial.println("Dispositivo já provisionado. Iniciando rotina normal...");
        diagnosticoCompleto();
        fazerLogin();
    }
}
