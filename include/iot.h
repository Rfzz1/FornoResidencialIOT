#ifndef IOT_H
#define IOT_H

void conectarWiFi();
void configurarNTP();
void printLocalTime();
void estabilizarHoraLocal();
String obterHorarioFormatado();
void verificarWiFi();
void taskConectividade(void *parameter);

#endif