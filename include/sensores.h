#ifndef SENSORES_H
#define SENSORES_H

float lerTemperatura();
bool temperaturaValida(double novaLeitura, bool compararComAnterior);
void taskTemperatura(void *parameter);

#endif