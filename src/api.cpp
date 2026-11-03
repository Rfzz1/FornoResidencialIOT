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
#include "certs.h"
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

    estadoForno estadoFornoCopia;

    uint32_t tempoLigadoMin = 0;

    static estadoForno estadoFornoAnterior = condicoes.estadoFornoAnterior;

    // Controle do encerramento da sessão.
    static bool encerramentoPendente = false;
    static bool telemetriaFinalEnviada = false;
    static unsigned long ultimaTentativaEncerramento = 0;

    const unsigned long INTERVALO_TENTATIVA_ENCERRAMENTO = 5000;

    eventoSistema evento;
    estadoForno eventoForno;

    for (;;)
    {
        // =====================================================
        // 1. PROCESSAR MUDANÇAS DO ESTADO DO FORNO
        // =====================================================

        while (xQueueReceive(eventosFornoQueue, &eventoForno, 0) == pdTRUE)
        {
            // Detectou que o forno ligou.
            if (estadoFornoAnterior == FORNO_DESLIGADO &&
                eventoForno != FORNO_DESLIGADO)
            {
                if (!encerramentoPendente && getSessaoId().isEmpty())
                {
                    Serial.println("[SESSAO] Solicitando início.");
                }
            }

            // Detectou que o forno desligou.
            if (eventoForno == FORNO_DESLIGADO &&
                estadoFornoAnterior != FORNO_DESLIGADO)
            {
                Serial.println("[SESSAO] Desligamento detectado.");

                encerramentoPendente = true;
                telemetriaFinalEnviada = false;
                ultimaTentativaEncerramento = 0;
            }

            estadoFornoAnterior = eventoForno;
        }

        // =====================================================
        // 2. ENVIAR EVENTOS DO SISTEMA
        // =====================================================

        while (xQueueReceive(eventosQueue, &evento, 0) == pdTRUE)
        {
            if (evento != NENHUM)
            {
                enviarEvento(obterEventoSistemaTexto(evento));
            }
        }

        // =====================================================
        // 3. CAPTURAR OS DADOS ATUAIS
        // =====================================================;

        xSemaphoreTake(mutexTelemetria, portMAX_DELAY);

        temperaturaAtualQueue = condicoes.TEMP_ATUAL;

        temperaturaUltimaQueue = condicoes.ULTIMA_TEMP;

        tempoLigadoMin = condicoes.tempoLigadoMinutos;

        estadoSistemaTexto = obterEstadoSistemaTexto(condicoes.estadoAtual);

        xSemaphoreGive(mutexTelemetria);

        xSemaphoreTake(mutexEstadoForno, portMAX_DELAY);

            estadoFornoCopia = condicoes.estadoFornoAtual;

        xSemaphoreGive(mutexEstadoForno);

        estadoFornoTexto = obterEstadoFornoTexto(estadoFornoCopia);

         bool fornoLigado = (estadoFornoTexto != "FORNO_DESLIGADO");

        // Segurança adicional:
        // Se o forno está desligado, mas ainda existe uma sessão aberta,
        // garante que o encerramento seja processado.
        if (!fornoLigado && !getSessaoId().isEmpty() && !encerramentoPendente)
        {
            encerramentoPendente = true;
            telemetriaFinalEnviada = false;
            ultimaTentativaEncerramento = 0;

            Serial.println("[SESSAO] Encerramento pendente identificado.");
        }

        // =====================================================
        // 4. PROCESSAR O ENCERRAMENTO PENDENTE
        // =====================================================

        if (encerramentoPendente)
        {
            if (getSessaoId().isEmpty())
            {
                Serial.println("[SESSAO] Não existe ID para encerrar.");

                encerramentoPendente = false;
                telemetriaFinalEnviada = false;
            }
            else
            {
                // Envia a telemetria final apenas uma vez.
                if (!telemetriaFinalEnviada)
                {
                    Serial.println("[SESSAO] Enviando telemetria final...");

                    enviarTelemetria(
                        temperaturaAtualQueue,
                        temperaturaUltimaQueue,
                        estadoSistemaTexto,
                        "FORNO_DESLIGADO",
                        tempoLigadoMin
                    );

                    telemetriaFinalEnviada = true;
                }

                // Tenta encerrar imediatamente ou após 5 segundos.
                if (ultimaTentativaEncerramento == 0 ||
                    millis() - ultimaTentativaEncerramento >= INTERVALO_TENTATIVA_ENCERRAMENTO)
                {
                    ultimaTentativaEncerramento = millis();

                    Serial.println("[SESSAO] Tentando encerrar pela API...");

                    if (encerrarSessao(estadoSistemaTexto, estadoFornoTexto))
                    {
                        Serial.println("[SESSAO] Encerramento concluído!");

                        encerramentoPendente = false;
                        telemetriaFinalEnviada = false;
                        ultimaTentativaEncerramento = 0;
                    }
                    else
                    {
                        Serial.println("[SESSAO] Encerramento falhou. Nova tentativa em 5 segundos.");
                    }
                }
            }
        }

        // =====================================================
        // 5. ATUALIZAÇÕES NORMAIS ENQUANTO O FORNO ESTÁ LIGADO
        // =====================================================

        if (fornoLigado && !encerramentoPendente)
        {
            // Atualiza o cronômetro e a sessão.
            // Qualquer HTTP executado aqui permanece na taskNuvem.
            tratarTempoSessao();

            xSemaphoreTake(mutexTelemetria, portMAX_DELAY);

            tempoLigadoMin = condicoes.tempoLigadoMinutos;

            xSemaphoreGive(mutexTelemetria);

            // Garante que exista uma sessão antes das atualizações.
            if (getSessaoId().isEmpty())
            {
                Serial.println("[SESSAO] Tentando recuperar ou criar sessão.");

                if (!iniciarSessao()) 
                {
                    Serial.println("[SESSAO] Não foi possível iniciar. Atualizações adiadas.");
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                    continue;
                }
            }


            bool enviada = enviarTelemetria(
                temperaturaAtualQueue,
                temperaturaUltimaQueue,
                estadoSistemaTexto,
                estadoFornoTexto,
                tempoLigadoMin
            );

            if (!enviada)
            {
                Serial.println("[SESSAO] Falha na telemetria final. Nova tentativa será feita.");
            }
        }

            enviarTemperatura(
                    estadoFornoTexto,
                    temperaturaAtualQueue,
                    temperaturaUltimaQueue
            );

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

    client.setCACert(ROOT_CA);

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

    String urlAuth = String(API_BASE_URL) + "/v1/fornos/auth";
    Serial.print("Tentando fazer login na URL: ");
    Serial.println(urlAuth);

    if (!iniciarHttp(http, client, urlAuth))
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

        String novoToken = res["token"].as<String>();

        // Atualiza a variável global principal que o enviarRequisicaoHTTP lê
        setTokenJWT(novoToken); 

        xSemaphoreTake(mutexLoginWebSocket, portMAX_DELAY);
        tokenJWT = novoToken;                // Atualiza diretamente também
        condicoes.tokenUsuario = novoToken;
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
    String *response = nullptr)
{
    // Garante o login ANTES de adquirir o mutex e instanciar os clientes de rede.
    // Isso evita abrir instâncias aninhadas de WiFiClientSecure que geram o erro -1.
    if (!garantirLogin())
    {
        return -1;
    }

    int tentativas = 0;
    const int MAX_TENTATIVAS = 3;
    bool sucesso = false;
    int codigo = -2;

    while (tentativas < MAX_TENTATIVAS && !sucesso)
    {
        xSemaphoreTake(mutexHttps, portMAX_DELAY);

        WiFiClientSecure secureClient;
        secureClient.setInsecure(); // Mantém temporariamente para isolar problemas de certificado
        HTTPClient http;

        Serial.print("URL: ");
        Serial.println(url);

        if (!iniciarHttp(http, secureClient, url))
        {
            http.end();
            xSemaphoreGive(mutexHttps);
            return -3; // Erro de conexão
        }

        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", "Bearer " + getTokenJWT());

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
            // Limpa o token global
            setTokenJWT("");

            xSemaphoreTake(mutexLoginWebSocket, portMAX_DELAY);
            condicoes.tokenUsuario = "";
            condicoes.fezLogin = false;
            xSemaphoreGive(mutexLoginWebSocket);
            
            // Fecha o http e libera o mutex ANTES de refazer o login
            http.end();
            xSemaphoreGive(mutexHttps);

            // Realiza o novo login com os sockets limpos
            fazerLogin();
            tentativas++;
            continue; // Repete o loop com o novo token
        }
        else if (codigo == 404)
        {
            Serial.println("AVISO: Recebido HTTP 404. Invalidando sessaoId atual.");
            setSessaoId("");
            tentativas++;
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

    if (!getSessaoId().isEmpty())
        return true;

    String resposta;
    
    Serial.print("Heap livre antes de iniciar sessão: ");
    Serial.println(ESP.getFreeHeap());

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/iniciar",
        "POST",
        "",
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

bool encerrarSessao(const String& estadoSistemaFinal, const String& estadoFornoFinal)
{

    String idSessao = getSessaoId();

    if (idSessao.isEmpty()) {
        return true;
    }

    String body;
    JsonDocument doc;

    doc["estadoSistemaFinal"] = estadoSistemaFinal;
    doc["estadoFornoFinal"] = estadoFornoFinal;

    serializeJson(doc, body);

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/sessoes/" + idSessao + "/encerrar",
        "PUT",
        body,
        nullptr);

    Serial.printf("Encerrar sessão: %d\n", code);

    if (code >= 200 && code < 300) {

        Serial.println("[SESSAO] Encerramento confirmado pela API.");

            setSessaoId("");

        return true;
    }

    Serial.println("[SESSAO] Falha ao encerrar. ID preservado para nova tentativa.");

    return false;
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
    Serial.println("Resposta:");
    Serial.println(resposta);
}

// ==========================
// ENVIAR TELEMETRIA
// ==========================

bool enviarTelemetria(double temperaturaAtual, double temperaturaUltima, const String &estadoSistema, const String &estadoForno, int tempoLigado)
{
    if (getSessaoId().isEmpty())
    {
        if (estadoForno == "FORNO_DESLIGADO")
        {
            return false;
        }
        
        // Tenta iniciar apenas se necessário
        if (!iniciarSessao())
        {
            Serial.println("Nao foi possivel iniciar a sessao.");
            return false;
        }
    }

    double temperaturaParaEnviar = temperaturaAtual;

    if (isnan(temperaturaParaEnviar) || temperaturaParaEnviar <= 0)
    {
        if (!isnan(temperaturaUltima) && temperaturaUltima > 0)
        {
            temperaturaParaEnviar = temperaturaUltima;
        }
        else
        {
            Serial.println("[TELEMETRIA] Nenhuma temperatura valida disponível.");
            return false;
        }
    }

    JsonDocument doc;
    String body;
    String resposta;

    doc["temperaturaAtual"] = temperaturaParaEnviar;
    doc["temperaturaUltima"] = (isnan(temperaturaUltima) || temperaturaUltima <= 0) ? 0.0 : temperaturaUltima;
    
    doc["estadoSistema"] = estadoSistema;
    doc["estadoForno"] = estadoForno;
    doc["tempoLigadoMinutos"] = tempoLigado;

    serializeJson(doc, body);

    int code = enviarRequisicaoHTTP(
        String(API_BASE_URL) + "/v1/telemetrias",
        "POST",
        body,
        &resposta);

    Serial.printf("Código telemetria: %d\n", code);
    Serial.println("Resposta:");
    Serial.println(resposta);

    return true;
}

// ==========================
// TEMPERATURA
// ==========================

void enviarTemperatura(const String &estadoFornoTexto, double temperaturaAtual, double temperaturaUltima)
{
    // Se o forno estiver desligado e não tem sessão, não faz nada
    if (estadoFornoTexto == "FORNO_DESLIGADO" && getSessaoId().isEmpty())
    {
        return;
    }

    if (isnan(temperaturaAtual) || temperaturaAtual <= 0)
    {
        return; // Não envia lixo para o backend se o sensor falhou
    }
    
    // Se não tem sessão ativa, tenta iniciar APENAS UMA VEZ
    if (getSessaoId().isEmpty())
    {
        if (!iniciarSessao())
        {
            Serial.println("Nao foi possivel iniciar a sessao.");
            return;
        }
    }

    Serial.printf("Heap = %u\n", ESP.getFreeHeap());
    Serial.print("Sessao atual: ");
    Serial.println(getSessaoId());

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

    Serial.printf("Código envio temperatura: %d\n", code);
    Serial.println("Resposta:");
    Serial.println(resposta);
}

// ==========================
// EVENTOS
// ==========================

void enviarEvento(String tipo)
{

    if (!iniciarSessao() || getSessaoId().isEmpty())
    {
        Serial.println("Falha ao enviar evento: Sessao não iniciada ou sessaoId vazio.");
        return;
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
