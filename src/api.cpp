#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

String tokenJWT = "";
String sessaoId = "";
String serialNumber;

void fazerLogin() {
  HTTPClient http; //Inicializa o cliente HTTP
  http.begin("http://192.168.0.129:8080/v1/auth/login"); //Rota de login da API
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON
 
  String body = "{\"email\":\"rafael@email.com\",\"senha\":\"123456\"}";

  int httpResponseCode = http.POST(body); //Envia a requisição POST para a API

  // Verifica a resposta da API (código de status HTTP)
  if (httpResponseCode == 200) {
    String response = http.getString(); //Obtém a resposta da API como string
    JsonDocument doc; //Cria um documento JSON para armazenar a resposta
    deserializeJson(doc, response); //Deserializa a resposta JSON para o documento
    tokenJWT = doc["token"].as<String>(); //Extrai o token JWT da resposta e armazena na variável global
    Serial.println("Login bem-sucedido. Token JWT recebido."); 
  } else {
    Serial.print("Erro no login: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void enviarSerialNumber() {
    serialNumber = WiFi.macAddress();

    serialNumber.replace(":", "");

    JsonDocument doc;

    doc["serialNumber"] = serialNumber;

    String body;

    serializeJson(doc, body);

    Serial.println(body);

    HTTPClient http;

    http.begin("http://192.168.0.129:8080/v1/fornos/registrar");

    http.addHeader("Content-Type", "application/json");

    int responseCode = http.POST(body);
}