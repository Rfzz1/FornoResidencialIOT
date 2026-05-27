#ifndef IOT_H
#define IOT_H

void conectarWiFi();
void configurarNTP();
void enviarBlynk();
void printLocalTime();
void estabilizarHoraLocal();
void verificarHorarioAlarme();
void atualizarEnvioBlynk();
void atualizarHorarioAlarme();
String obterHorarioFormatado();
BLYNK_WRITE(V5);

#endif