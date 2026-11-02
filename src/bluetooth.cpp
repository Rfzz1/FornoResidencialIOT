#include "bluetooth.h"
#include <Preferences.h>
#include "BluetoothSerial.h"
#include "telemetria.h"
#include "config.h"

BluetoothSerial SerialBT;
static bool aguardandoReinicio = false;
static unsigned long tempoInicioReinicio = 0;
static bool bluetoothConectado = false;
static bool deveDesligar = false;

void taskBluetooth(void *parameter)
{
    unsigned long desligamento = millis();

    for (;;)
    {
        if (millis() - desligamento >= 120000 && bluetoothConectado) {
            solicitarDesligamentoBluetooth(); 
        }

        if (deveDesligar)
        {
            Serial.println("Desligando Bluetooth para liberar memória.... ");
            SerialBT.end();
            bluetoothConectado = false;
            vTaskDelete(NULL);
        }

        if (bluetoothConectado == true)
        {
            processarBluetooth();
            verificarReiniciar();
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void solicitarDesligamentoBluetooth()
{
    deveDesligar = true;
}

void salvarSecretBluetooth(String recebidoDoTerminal)
{
    String secretLimpa = "";
    for (char c : recebidoDoTerminal)
    {
        if (isAlphaNumeric(c) || c == '-')
        {
            secretLimpa += c;
        }
    }
    // Salva a versão limpa nas Preferences
    preferences.putString("secret", secretLimpa);
    condicoes.deviceSecret = secretLimpa;
    Serial.println("Nova secret salva com sucesso!");
}

void inicializarBluetooth()
{
    SerialBT.begin("MonitorForno2");
    SerialBT.setPin("123456");
    bluetoothConectado = true;
    Serial.println("Bluetooth iniciado. Requer PIN para pareamento.");
}

void inicializarPreferences()
{

    Serial.println("Abrindo Preferences...");

    if (!preferences.begin("forno", false))
    {
        Serial.println("Falha ao iniciar Preferences");
        return;
    }

    condicoes.serialNumber = preferences.getString("serialNumber", "");
    condicoes.deviceSecret = preferences.getString("secret", "");

    Serial.println("Secret salva");

    if (condicoes.serialNumber.isEmpty())
    {
        condicoes.serialNumber = WiFi.macAddress();
        condicoes.serialNumber.replace(":", "");
        preferences.putString("serialNumber", condicoes.serialNumber);
    }

    Serial.println("Serial Number salvo");
}

// ==========================
// BLUETOOTH PROCESS
// ==========================

void processarBluetooth()
{

    static bool conectado = false;

    if (SerialBT.hasClient() && !conectado)
    {
        conectado = true;
        Serial.println("Bluetooth conectado");
    }

    if (!SerialBT.hasClient())
    {
        conectado = false;
        return;
    }

    if (!SerialBT.available())
        return;

    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();

    if (cmd == "GET_SERIAL")
    {

        String s = WiFi.macAddress();
        s.replace(":", "");
        SerialBT.println(s);
        return;
    }

    if (cmd.startsWith("SET_SECRET:"))
    {

        condicoes.deviceSecret = cmd.substring(11);
        preferences.putString("secret", condicoes.deviceSecret);

        SerialBT.println("OK");

        tempoInicioReinicio = millis();
        aguardandoReinicio = true;
    }

    if (cmd == "CLEAR_SECRET")
    {
        preferences.remove("secret");
        SerialBT.println("SECRET_REMOVIDO");
        delay(1000);
        ESP.restart();
    }
}

// ==========================
// CONTROLE
// ==========================

void verificarReiniciar()
{
    if (aguardandoReinicio &&
        millis() - tempoInicioReinicio >= 2000)
    {
        ESP.restart();
    }
}