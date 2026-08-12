#ifndef WS_H
#define WS_H

// Inicializa e conecta o WebSocket (chamado no setup ou após conectar no Wi-Fi)
void inicializarWebSocket();

// Mantém o canal aberto e gerencia reconexões (chamado no loop)
void processarWebSocket();

#endif