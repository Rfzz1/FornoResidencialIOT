#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include "BluetoothSerial.h"

#include "config.h"
#include "telemetria.h"
#include "iot.h"
#include "estados.h"
#include "sessao.h"
#include "alertas.h"
#include "logs.h"
#include "eventos.h"
#include "sensores.h"
#include "api.h"

// ==========================
// OBJETOS GLOBAIS
// ==========================

BluetoothSerial SerialBT;
Preferences preferences;

String tokenJWT = "";
String sessaoId = "";
String serialNumber;
String deviceSecret;

String body;
double temperaturaAtual = 0;
double temperaturaUltima = 0;

static bool aguardandoReinicio = false;
static unsigned long tempoInicioReinicio = 0;

// controle de recuperação de sessão
static bool tentandoRecuperarSessao = false;
static int tentativasSessao = 0;


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
    Serial.println("\n[TESTE 4] HTTPS direto no IP 56.125.180.47...");
    {
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();
        http.begin(client, "https://56.125.180.47/v1/fornos/auth");
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
    String secretLimpa = deviceSecret;
    secretLimpa.trim(); 
    if (secretLimpa.length() > 36) {
        secretLimpa = secretLimpa.substring(0, 36);
    }
    doc["secret"] = secretLimpa;
    doc["serialNumber"] = serialNumber;

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
    deviceSecret = secretLimpa;
    Serial.println("Nova secret salva com sucesso!");
}

// ==========================
// GARANTIA LOGIN
// ==========================

bool garantirLogin() {

    if (!tokenJWT.isEmpty())
        return true;

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
    String *response
) {

    HTTPClient http;

    if (url.startsWith("https://")) {

    WiFiClientSecure client;
    client.setInsecure();

    http.begin(client, url);

    } else {

        WiFiClient client;

        http.begin(client, url);
    }

    if (!garantirLogin())
        return -1;

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + tokenJWT);

    int codigo;
    
    if (metodo == "POST") codigo = http.POST(payload);
    else if (metodo == "PUT") codigo = http.PUT(payload);
    else if (metodo == "GET") codigo = http.GET();
    else {
        http.end();
        return -2;
    }

    Serial.printf("HTTP retornou: %d\n", codigo);

    String respostaHttp;

    if (codigo > 0) {
        respostaHttp = http.getString();
        Serial.println("Resposta da API:");
        Serial.println(respostaHttp);
    } else {
        Serial.println(http.errorToString(codigo));
    }

    if (response != nullptr) {
        *response = respostaHttp;
    }
    http.end();

    // =========================
    // TOKEN INVÁLIDO
    // =========================

    if (codigo == 401) {

        Serial.println("Token expirado. Refazendo login...");

        tokenJWT = "";

        if (!garantirLogin())
            return codigo;

        return enviarRequisicaoHTTP(url, metodo, payload, response);
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

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/" + sessaoId + "/encerrar",
        "PUT",
        body,
        nullptr
    );

    Serial.printf("Encerrar sessão: %d\n", code);
}

// ==========================
// TEMPERATURA
// ==========================

void enviarTemperatura() {

    JsonDocument doc;

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
    doc["temperaturaAtual"] = dados.TEMP_ATUAL;
    doc["temperaturaUltima"] = dados.ULTIMA_TEMP;
    doc["temperaturaExterna"] = dados.TEMP_EXT_ATUAL;

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

    JsonDocument doc;

    doc["sessaoId"] = sessaoId;
    doc["tipo"] = tipo;

    serializeJson(doc, body);

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/eventos",
        "POST",
        body,
        nullptr
    );

    Serial.printf("Evento: %d\n", code);
}

// ==========================
// BLUETOOTH
// ==========================

void inicializarBluetooth() {
    SerialBT.begin("MonitorForno2");
    Serial.println("Bluetooth iniciado.");
}

void inicializarPreferences() {

    Serial.println("Abrindo Preferences...");

    if (!preferences.begin("forno", false)) {
        Serial.println("Falha ao iniciar Preferences");
        return;
    }

    serialNumber = preferences.getString("serialNumber", "");
    deviceSecret = preferences.getString("secret", "");

    Serial.println("Secret salva:");
    Serial.println(deviceSecret);

    if (serialNumber.isEmpty()) {
        serialNumber = WiFi.macAddress();
        serialNumber.replace(":", "");
        preferences.putString("serialNumber", serialNumber);
    }


    Serial.println("Serial: " + serialNumber);
}

// ==========================
// BLUETOOTH PROCESS
// ==========================

void processarBluetooth() {

    static bool conectado = false;

    if (SerialBT.hasClient() && !conectado) {
        conectado = true;
        Serial.println("Bluetooth conectado");
    }

    if (!SerialBT.hasClient()) {
        conectado = false;
        return;
    }

    if (!SerialBT.available())
        return;

    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();

    if (cmd == "GET_SERIAL") {

        String s = WiFi.macAddress();
        s.replace(":", "");
        SerialBT.println(s);
        return;
    }

    if (cmd.startsWith("SET_SECRET:")) {

        deviceSecret = cmd.substring(11);
        preferences.putString("secret", deviceSecret);

        SerialBT.println("OK");

        tempoInicioReinicio = millis();
        aguardandoReinicio = true;
    }

    if (cmd == "CLEAR_SECRET") {
        preferences.remove("secret");
        SerialBT.println("SECRET_REMOVIDO");
        delay(1000);
        ESP.restart();
    }
}

// ==========================
// CONTROLE
// ==========================

void verificarReiniciar() {
    if (aguardandoReinicio &&
        millis() - tempoInicioReinicio >= 2000) {
        ESP.restart();
    }
}

void verificarEstadoDispositivo() {

    if (preferences.getString("secret", "").isEmpty()) {
        dados.espConfigurado = false;
        Serial.println("Aguardando configuração Bluetooth...");
    } else {
        dados.espConfigurado = true;
        conectarWiFi();
        diagnosticoCompleto();
        fazerLogin();
    }
}

void gerenciarEstadoOperacional() {

    processarBluetooth();

    if (!dados.espConfigurado) {
        return;
    }

    if (tokenJWT.isEmpty() && WiFi.status() == WL_CONNECTED) {
        fazerLogin();
    }

    atualizarSensores();
    atualizarEstadoSistema();
    atualizarEstadoForno();

    processarEventos();
    processarFilaEventos();

    tratarSessao();

    atualizarAlertas();
    atualizarHorarioAlarme();
    atualizarEnvioLogs();
}

// ==========================
// TEMPERATURAS
// ==========================

void sincronizarTemperaturas() {
    temperaturaAtual = dados.TEMP_ATUAL;
    temperaturaUltima = dados.ULTIMA_TEMP;
    enviarTemperatura();
}

// ==========================
// SERIAL
// ==========================

String obterSerialNumber() {
    return serialNumber;
}