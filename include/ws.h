#ifndef WS_H
#define WS_H

#include <WebSocketsClient.h>

void aoReceberEventoWebSocket(WStype_t tipoEvento, uint8_t * texto, size_t tamanho);
void taskWebSocket(void *parameter);

#endif