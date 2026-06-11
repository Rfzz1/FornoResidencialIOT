#ifndef IOT_H
#define IOT_H

void conectarWiFi();
void configurarNTP();
void enviarBlynk();
void printLocalTime();
void estabilizarHoraLocal();
void verificarHorarioAlarme();
void atualizarEnvioBlynk();
void conectarBlynk();
void iniciarBlynk();
void atualizarHorarioAlarme();
String obterHorarioFormatado();

#endif