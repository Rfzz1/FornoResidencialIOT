#include "bluetooth.h"
#include <Preferences.h>
#include "BluetoothSerial.h"
#include "telemetria.h"

BluetoothSerial SerialBT;
Preferences preferences;
static bool aguardandoReinicio = false;
static unsigned long tempoInicioReinicio = 0;
static bool bluetoothConectado = false;

void taskBluetooth(void *parameter) {

    for (;;) {

        if (bluetoothConectado == true) {
            processarBluetooth();
            verificarReiniciar();
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

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


void inicializarBluetooth() {
    SerialBT.begin("MonitorForno2");
    bluetoothConectado = true;
    Serial.println("Bluetooth iniciado.");
}

void inicializarPreferences() {

    Serial.println("Abrindo Preferences...");

    if (!preferences.begin("forno", false)) {
        Serial.println("Falha ao iniciar Preferences");
        return;
    }

    dados.serialNumber = preferences.getString("serialNumber", "");
    dados.deviceSecret = preferences.getString("secret", "");

    Serial.println("Secret salva:");
    Serial.println(dados.deviceSecret);

    if (dados.serialNumber.isEmpty()) {
        dados.serialNumber = WiFi.macAddress();
        dados.serialNumber.replace(":", "");
        preferences.putString("serialNumber", dados.serialNumber);
    }


    Serial.println("Serial: " + dados.serialNumber);
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

        dados.deviceSecret = cmd.substring(11);
        preferences.putString("secret", dados.deviceSecret);

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