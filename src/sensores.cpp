#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "sensores.h"
#include "telemetria.h"
#include "api.h"

static unsigned long ultimaLeituraSensor = 0;
static unsigned long ultimoEnvioTemperatura = 0;
static double leituraBruta = 0;

const int TAM_MEDIA = 10;

double bufferTemperaturas[TAM_MEDIA];

int indiceBuffer = 0;

bool bufferCheio = false;

MAX6675 TERMOPAR(TERMOPAR_SCK, TERMOPAR_CS, TERMOPAR_SO);

void taskTemperatura(void *parameter)
{

  for (;;)
  {

    double temperaturaAtual = lerTemperatura();
    xQueueOverwrite(temperaturaQueue, &temperaturaAtual);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

float lerTemperatura()
{

  leituraBruta = TERMOPAR.readCelsius();

  if (isnan(leituraBruta) || leituraBruta <= 1.0 || leituraBruta > 1000.0)
  {
    Serial.println("Interferência detectada! Leitura ignorada.");
    return NAN;
  }

  bufferTemperaturas[indiceBuffer] = leituraBruta;

  indiceBuffer++;

  if (indiceBuffer >= TAM_MEDIA)
  {

    indiceBuffer = 0;
    bufferCheio = true;
  }

  int quantidade =
      bufferCheio ? TAM_MEDIA : indiceBuffer;

  float soma = 0;

  for (int i = 0; i < quantidade; i++)
  {

    soma += bufferTemperaturas[i];
  }

  double temperaturaAtual = soma / quantidade;

  Serial.printf("Leitura bruta do sensor: %.2f °C\n", temperaturaAtual);
  return temperaturaAtual;
}

bool temperaturaValida(double novaTemperatura)
{
  if (isnan(novaTemperatura))
  {
    Serial.println("Erro: Falha ao ler sensor!");
    return false;
  }

  if (novaTemperatura < 0)
  {
    Serial.println("Erro: Temperatura negativa!");
    return false;
  }

  if (novaTemperatura > TEMP_MAXIMA_VALIDA)
  {
    Serial.println("Erro: Temperatura muito alta!");
    return false;
  }

  // SE o sistema estava em erro, permitimos que a primeira leitura válida passe 
  if (condicoes.estadoAtual == ERRO_SENSOR)
  {
    condicoes.ULTIMA_TEMP = novaTemperatura; // Usa o novo valor
    return true;
  }

  // Compara a nova com a antiga guardada
  if (fabs(novaTemperatura - condicoes.ULTIMA_TEMP) > 100)
  {
    Serial.println("Erro: Variacao abrupta!");
    return false;
  }

  return true;
}