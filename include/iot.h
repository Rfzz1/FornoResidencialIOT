#ifndef IOT_H
#define IOT_H

void conectarWiFi();
void configurarNTP();
void printLocalTime();
void estabilizarHoraLocal();
void verificarHorarioAlarme();
void atualizarHorarioAlarme();
String obterHorarioFormatado();
void verificarWiFi();
void taskConectividade(void *parameter);

#endif