#include "bluetooth.h"
#include <Preferences.h>
#include "BluetoothSerial.h"
#include "esp_bt.h"
#include "telemetria.h"
#include "config.h"

BluetoothSerial SerialBT;
static bool aguardandoReinicio = false;
static unsigned long tempoInicioReinicio = 0;
static bool bluetoothConectado = false;
static bool deveDesligar = false;

// Só por usar a BluetoothSerial, o ESP32 deixa a memória do controlador Bluetooth
// reservada o tempo todo (dezenas de KB), mesmo com o Bluetooth desligado.
// O TLS (HTTPS + WebSocket) precisa dessa memória. Como o Bluetooth só é usado
// no provisionamento, devolvemos essa memória assim que ele não é mais necessário.
// Depois disso o Bluetooth só volta a funcionar após reiniciar a placa.
static void liberarMemoriaBluetooth()
{
    static bool liberada = false;
    if (liberada) return;

    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_IDLE)
    {
        Serial.println("[BT] Controlador ainda ativo, memoria nao liberada.");
        return;
    }

    liberada = (esp_bt_controller_mem_release(ESP_BT_MODE_BTDM) == ESP_OK);
    Serial.printf("[BT] Memoria do Bluetooth liberada: %s | heap livre=%u | maior bloco=%u\n",
                  liberada ? "sim" : "nao", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
}

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
            liberarMemoriaBluetooth();
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

    // Bluetooth nunca foi ligado (forno já provisionado): libera agora,
    // antes do primeiro HTTPS, sem esperar a taskBluetooth.
    if (!bluetoothConectado)
    {
        liberarMemoriaBluetooth();
    }
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