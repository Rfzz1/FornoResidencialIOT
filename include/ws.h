#ifndef WS_H
#define WS_H

#include <WebSocketsClient.h>

struct MensagemWS { char payload[160]; };

void aoReceberEventoWebSocket(WStype_t tipoEvento, uint8_t * texto, size_t tamanho);
void taskWebSocket(void *parameter);
bool enviarEventoPeloWebSocket(const char *tipoEvento, const String &sessaoId);

#endif