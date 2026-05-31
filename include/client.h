#ifndef CLIENT_MINERS_H
#define CLIENT_MINERS_H

#include "protocol.h"
#include <stdbool.h>

typedef struct client_ctx client_ctx_t;

// constructors
client_ctx_t *client_connect(const char *host, const char *port,
                             const char *username);
void client_destroy(client_ctx_t *client);

// messaging
bool client_send_text(client_ctx_t *client, const char *msg);
bool client_disconnect(client_ctx_t *client);

// receiving
int client_recv(client_ctx_t *client);
bool client_next_msg(client_ctx_t *client, msg_t *msg);

// helpers
int client_getfd(client_ctx_t *client);
const char *client_get_username(client_ctx_t *client);

#endif
