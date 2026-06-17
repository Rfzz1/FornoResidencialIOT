#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "BluetoothSerial.h"
#include "config.h"

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

void fazerLogin() {

  doc.clear();
  
  http.begin("http://192.168.0.129:8080/v1/fornos/auth"); //Rota de login da API
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

  doc.clear();

  http.begin("http://192.168.0.129:8080/v1/sessoes/iniciar"); //Rota de início de sessão da API
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON

  if(tokenJWT.isEmpty()){
    Serial.println("Usuário não autenticado");
    return;
  }

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
  }

  http.end();

}

void encerrarSessao() {

  doc.clear();

  http.begin("http://192.168.0.129:8080/v1/sessoes/" + sessaoId + "/encerrar"); //Rota para encerrar sessão
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON

  if(tokenJWT.isEmpty()){
      Serial.println("Usuário não autenticado");
      return;
  }

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

  doc.clear();

  http.begin("http://192.168.0.129:8080/v1/temperaturas"); //Rota para enviar temperatura
  http.addHeader("Content-Type", "application/json"); //Define o tipo de conteúdo como JSON

  if(tokenJWT.isEmpty()){
    Serial.println("Usuário não autenticado");
    return;
  }

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
  }

  http.end();
}

void inicializarPreferences() {
    if (!preferences.begin("forno", false)) {
        Serial.println("Falha ao iniciar Preferences");
        return;
    }

    serialNumber = preferences.getString("serialNumber", "");
    deviceSecret = preferences.getString("secret", "");

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

        preferences.putString(
            "secret",
            deviceSecret
        );

        SerialBT.println("OK");

        return;
    }
}

String obterSerialNumber() {
  return serialNumber;
}