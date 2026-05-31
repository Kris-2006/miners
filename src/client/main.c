#include "../../include/client.h"
#include "../../include/common.h"
#include "../../include/protocol.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  if (argc < 3) {
    LOG_INFO("Usage: %s addr username [port]", argv[0]);
    return -1;
  }

  const char *host = argv[1];
  const char *username = argv[2];
  const char *port = (argc == 4) ? argv[3] : DEF_PORT;

  LOG_INFO("Connecting to %s:%s as '%s'...", host, port, username);
  client_ctx_t *ctx = client_connect(host, port, username);
  if (!ctx) {
    LOG_ERR("Connection failed");
    return -1;
  }
  LOG_INFO("Connected! fd=%d, user=%s", client_getfd(ctx),
           client_get_username(ctx));

  LOG_INFO("Sending text message...");
  if (!client_send_text(ctx, "Hello from the new client API!")) {
    LOG_ERR("Send failed");
    client_destroy(ctx);
    return -1;
  }
  LOG_INFO("Message sent");

  // recv  test..
  LOG_INFO("Waiting for a message from server (2s)...");
  sleep(2);

  int n = client_recv(ctx);
  if (n > 0) {
    LOG_INFO("Received %d bytes", n);
    msg_t msg;
    while (client_next_msg(ctx, &msg)) {
      LOG_INFO("  [%s] type=%d sender='%s' payload='%.*s'",
               msg.TYPE == MSG_TEXT     ? "TEXT"
               : msg.TYPE == MSG_STATUS ? "STATUS"
                                        : "OTHER",
               msg.TYPE, msg.sender, msg.len, msg.payload);
    }
  } else if (n == 0) {
    LOG_INFO("Server closed connection");
  } else {
    LOG_INFO("No data (recv returned %d)", n);
  }

  // Disconnect
  LOG_INFO("Disconnecting...");
  client_disconnect(ctx);
  client_destroy(ctx);
  LOG_INFO("Done.");

  return 0;
}
