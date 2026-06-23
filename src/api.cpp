#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
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

BluetoothSerial SerialBT;
Preferences preferences;

String tokenJWT = "";
String sessaoId = "";
String serialNumber;
String deviceSecret;
String body;
double temperaturaAtual;
double temperaturaUltima;
JsonDocument doc;
HTTPClient http; //Inicializa o cliente HTTP

static bool aguardandoReinicio = false;
static unsigned long tempoInicioReinicio = 0;

void fazerLogin() {

  doc.clear();
  
  http.begin("http://192.168.0.126:8080/v1/fornos/auth"); //Rota de login da API
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON

  doc["serialNumber"] = serialNumber; //Adiciona o número de série ao documento JSON
  doc["secret"] = deviceSecret; //Adiciona o segredo do dispositivo ao documento JSON

  body = "";
  serializeJson(doc, body); //Serializa o documento JSON para uma string

  int httpResponseCode = http.POST(body); //Envia a requisição POST para a API

  // Verifica a resposta da API (código de status HTTP)
  if (httpResponseCode == 200) {
    String response = http.getString(); //Obtém a resposta da API como string
    deserializeJson(doc, response); //Deserializa a resposta JSON para o documento
    tokenJWT = doc["token"].as<String>(); //Extrai o token JWT da resposta e armazena na variável global
    Serial.println("Login bem-sucedido. Token JWT recebido."); 
  } else {
    Serial.print("Erro no login: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void iniciarSessao() {

  
  if(tokenJWT.isEmpty()){
    Serial.println("Usuário não autenticado");
    return;
  }

  doc.clear();

  http.begin("http://192.168.0.126:8080/v1/sessoes/iniciar"); //Rota de início de sessão da API
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON
  http.addHeader("Authorization", "Bearer " + tokenJWT); //Adiciona o token JWT no cabeçalho de autorização

  body = "";
  int httpResponseCode = http.POST(body); //Envia a requisição POST para a API

  if (httpResponseCode == 201) {
    String response = http.getString(); //Obtém a resposta da API como string
    deserializeJson(doc, response); //Deserializa a resposta JSON para o documento
    sessaoId = doc["id"].as<String>(); //Extrai o ID da sessão da resposta e armazena na variável global
    Serial.println("Sessão iniciada com sucesso.");
  } else {
    Serial.print("Erro ao iniciar sessão: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode == 401 || httpResponseCode == 403) {
      tokenJWT = "";
      Serial.println("Token expirado ou inválido. Forçando novo login...");
    }
  }

  http.end();

}

void encerrarSessao() {

  
  if(tokenJWT.isEmpty()){
    Serial.println("Usuário não autenticado");
    return;
  }

  doc.clear();

  http.begin("http://192.168.0.126:8080/v1/sessoes/" + sessaoId + "/encerrar"); //Rota para encerrar sessão
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON

  http.addHeader("Authorization", "Bearer " + tokenJWT); //Adiciona o token JWT no cabeçalho de autorização

  body = "";
  int httpResponseCode = http.PUT(body); //Envia a requisição PUT para a API

  if (httpResponseCode == 200) {
    Serial.println("Sessão encerrada com sucesso.");
  } else {
    Serial.print("Erro ao encerrar sessão: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void enviarTemperatura() {

  
  if(tokenJWT.isEmpty()){
    Serial.println("Usuário não autenticado");
    return;
  }

  doc.clear();

  http.begin("http://192.168.0.126:8080/v1/temperaturas"); //Rota para enviar temperatura
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON

  http.addHeader("Authorization", "Bearer " + tokenJWT);

  doc["sessaoId"] = sessaoId; //Adiciona o ID da sessão ao documento JSON
  doc["temperaturaAtual"] = temperaturaAtual; //Adiciona a temperatura atual ao documento JSON
  doc["temperaturaUltima"] = temperaturaUltima; //Adiciona a última temperatura ao documento JSON

  body = "";
  serializeJson(doc, body); //Serializa o documento JSON para uma string
  int httpResponseCode = http.POST(body); //Envia a requisição POST para a API

  if (httpResponseCode == 201) {
    Serial.println("Temperatura enviada com sucesso.");
  } else {
    Serial.print("Erro ao enviar temperatura: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode == 401 || httpResponseCode == 403) {
      tokenJWT = "";  
      Serial.println("Token expirado ou inválido. Forçando novo login...");
    }
  }

  http.end();
}

void enviarEvento(String tipo) {

  
  if(tokenJWT.isEmpty()){
    Serial.println("Usuário não autenticado");
    return;
  }

  doc.clear();

  http.begin("http://192.168.0.126:8080/v1/eventos"); //Rota para enviar evento
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON

  http.addHeader("Authorization", "Bearer " + tokenJWT);

  doc["sessaoId"] = sessaoId; //Adiciona o ID da sessão ao documento JSON
  doc["tipo"] = tipo; //Adiciona o tipo de evento ao documento JSON

  body = "";
  serializeJson(doc, body); //Serializa o documento JSON para uma string
  int httpResponseCode = http.POST(body); //Envia a requisição POST para a API

  if (httpResponseCode == 201) {
    Serial.println("Evento enviado com sucesso.");
  } else {
    Serial.print("Erro ao enviar evento: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode == 401 || httpResponseCode == 403) {
        tokenJWT = "";
        Serial.println("Token expirado ou inválido. Forçando novo login...");
    }

  }

  http.end();

}

void inicializarPreferences() {
    if (!preferences.begin("forno", false)) {
        Serial.println("Falha ao iniciar Preferences");
        return;
    }

    serialNumber = preferences.getString("serialNumber", ""); //Tenta carregar o número de série salvo, se não existir, retorna uma string vazia
    deviceSecret = preferences.getString("secret", ""); //Tenta carregar o segredo do dispositivo salvo, se não existir, retorna uma string vazia

    //Se o número de série não existir, gera um novo a partir do MAC address do WiFi, remove os ":" e salva nas Preferences
    if (serialNumber.isEmpty()) {
        serialNumber = WiFi.macAddress();
        serialNumber.replace(":", "");
        preferences.putString("serialNumber", serialNumber);
        Serial.println("Número de série gerado e armazenado: " + serialNumber);
    }

    Serial.println("Serial carregado: " + serialNumber);

    Serial.println("Secret carregado: " + deviceSecret);
}

void processarBluetooth() {
      if(!SerialBT.available()) {
        return;
    }

    String comando =
        SerialBT.readStringUntil('\n');

    comando.trim();

    if(comando == "GET_SERIAL") {

        String serial =
            WiFi.macAddress();

        serial.replace(":", "");

        SerialBT.println(serial);

        return;
    }

    if(comando.startsWith("SET_SECRET:")) {

        String secret =
            comando.substring(11);

        deviceSecret = secret;
        dados.espConfigurado = true;

        preferences.putString(
            "secret",
            deviceSecret
        );

        SerialBT.println("OK");
        Serial.println("Configuração recebida. Reiniciando em 2 segundos...");

        tempoInicioReinicio = millis();
        aguardandoReinicio = true;
        return;
    }
}

void verificarReiniciar() {
    if (aguardandoReinicio && (millis() - tempoInicioReinicio >= 2000)) {
        ESP.restart();
    }
}

void verificarEstadoDispositivo() {
    
    String secretSalvo = preferences.getString("secret", "");

    if (secretSalvo.isEmpty()) {
      dados.espConfigurado = false;
      Serial.println("Dispositivo não configurado. Aguardando configuração via Bluetooth...");
    } else {
        Serial.println("Dispositivo configurado. Pronto para uso.");
        dados.espConfigurado = true;
        conectarWiFi();
        fazerLogin();
    }

}

void gerenciarEstadoOperacional() {
  if (!dados.espConfigurado) {
    processarBluetooth();
    //Adicionar um led piscando para indicar que o dispositivo está aguardando configuração
    return;
  } else {

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
    atualizarEnvioBlynk();
  }
}

void sincronizarTemperaturas() {
    temperaturaAtual = dados.TEMP_ATUAL;
    temperaturaUltima = dados.ULTIMA_TEMP;
    enviarTemperatura();
}

String obterSerialNumber() {
  return serialNumber;
}