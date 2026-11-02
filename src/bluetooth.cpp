#include "bluetooth.h"
#include <Preferences.h>
#include "BluetoothSerial.h"
#include "telemetria.h"
#include "config.h"

BluetoothSerial SerialBT;
static bool aguardandoReinicio = false;
static unsigned long tempoInicioReinicio = 0;
static bool bluetoothConectado = false;

void taskBluetooth(void *parameter)
{

    for (;;)
    {

        if (bluetoothConectado == true)
        {
            processarBluetooth();
            verificarReiniciar();
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void inicializarBluetooth()
{
    SerialBT.begin("MonitorForno2");
    bluetoothConectado = true;
    Serial.println("Bluetooth iniciado.");
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

    Serial.println("Secret salva:");
    Serial.println(condicoes.deviceSecret);

    if (condicoes.serialNumber.isEmpty())
    {
        condicoes.serialNumber = WiFi.macAddress();
        condicoes.serialNumber.replace(":", "");
        preferences.putString("serialNumber", condicoes.serialNumber);
    }

    Serial.println("Serial: " + condicoes.serialNumber);
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