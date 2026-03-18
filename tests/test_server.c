#include "../include/server.h"
#include <assert.h>
#include <unistd.h>

int main() {
  server_ctx_t *server;
  server = server_ctx_init();
  assert(server != NULL);
  assert(client_add(server, 10) == 0);
  client_t *c = clientfd_find(server, 10);
  assert(c != NULL);
  assert(c->fd == 10);
  client_auth(server, c, "Alice");
  client_t *new = clientname_find(server, "Alice");
  assert(new->auth == 1);
  assert(new == c);
  client_remove(server, 10);
  assert(clientfd_find(server, 10) == NULL);
  assert(clientname_find(server, "Alice") == NULL);

  server_ctx_free(server);
  return 0;
}
