#ifndef SENSORES_H
#define SENSORES_H

float lerTemperatura();
void lerTemperaturaExterna();
void atualizarSensores();
bool temperaturaValida();
void taskTemperatura(void *parameter);

#endif