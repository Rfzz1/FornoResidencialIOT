#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

String tokenJWT = "";
String sessaoId = "";
String serialNumber;
String email;
String senha;
String body;
String temperaturaAtual;
String temperaturaUltima;
JsonDocument doc;
HTTPClient http; //Inicializa o cliente HTTP

void fazerLogin() {

  doc.clear();
  
  http.begin("http://192.168.0.129:8080/v1/auth/login"); //Rota de login da API
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON

  doc["email"] = email; //Adiciona o email ao documento JSON
  doc["senha"] = senha; //Adiciona a senha ao documento JSON

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

  doc.clear();

  http.begin("http://192.168.0.129:8080/v1/sessoes/iniciar"); //Rota de início de sessão da API
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON
  http.addHeader("Authorization", "Bearer " + tokenJWT); //Adiciona o token JWT no cabeçalho de autorização

  int httpResponseCode = http.POST(body); //Envia a requisição POST para a API

  if (httpResponseCode == 201) {
    String response = http.getString(); //Obtém a resposta da API como string
    deserializeJson(doc, response); //Deserializa a resposta JSON para o documento
    sessaoId = doc["id"].as<String>(); //Extrai o ID da sessão da resposta e armazena na variável global
    Serial.println("Sessão iniciada com sucesso.");
  } else {
    Serial.print("Erro ao iniciar sessão: ");
    Serial.println(httpResponseCode);
  }

  http.end();

}

void encerrarSessao() {

  doc.clear();

  http.begin("http://192.168.0.129:8080/v1/sessoes/" + sessaoId + "/encerrar"); //Rota para encerrar sessão
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON
  http.addHeader("Authorization", "Bearer " + tokenJWT); //Adiciona o token JWT no cabeçalho de autorização

  int httpResponseCode = http.PUT(""); //Envia a requisição PUT para a API

  if (httpResponseCode == 200) {
    Serial.println("Sessão encerrada com sucesso.");
  } else {
    Serial.print("Erro ao encerrar sessão: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void enviarTemperatura() {

  doc.clear();

  http.begin("http://192.168.0.129:8080/v1/temperaturas"); //Rota para enviar temperatura
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON
  http.addHeader("Authorization", "Bearer " + tokenJWT); //Adiciona o

  doc["sessaoId"] = sessaoId; //Adiciona o ID da sessão ao documento JSON
  doc["temperaturaAtual"] = temperaturaAtual; //Adiciona a temperatura atual ao documento JSON
  doc["temperaturaUltima"] = temperaturaUltima; //Adiciona a última temperatura ao documento JSON
  doc["email"] = email; //Adiciona o email ao documento JSON

  serializeJson(doc, body); //Serializa o documento JSON para uma string
  int httpResponseCode = http.POST(body); //Envia a requisição POST para a API

  if (httpResponseCode == 201) {
    Serial.println("Temperatura enviada com sucesso.");
  } else {
    Serial.print("Erro ao enviar temperatura: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void enviarSerialNumber() {

  doc.clear();

  serialNumber = WiFi.macAddress();

  serialNumber.replace(":", "");

  doc["serialNumber"] = serialNumber;

  serializeJson(doc, body);

  Serial.println(body);

  http.begin("http://192.168.0.129:8080/v1/fornos/registrar");

  http.addHeader("Content-Type", "application/json");

  int responseCode = http.POST(body);
}