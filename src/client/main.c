#include <signal.h>
#include <sys/poll.h>
#include <unistd.h>

#include "client.h"
#include "common.h"
#include "msglist.h"
#include "protocol.h"
#include "ui.h"

volatile sig_atomic_t g_running = 1;

void h_sigint(int sig) {
    (void)sig;
    g_running = 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        LOG_ERR("Usage: %s host username [port]\n", argv[0]);
        return -1;
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = h_sigint;
    sigaction(SIGINT, &sa, NULL);

    char const* host = argv[1];
    char const* username = argv[2];
    char const* port = (argc == 4) ? argv[3] : DEF_PORT;

    chat_msg* msgList = chat_init();
    if (!msgList) {
        LOG_ERR("Chat Not initialised");
        return -1;
    }
    client_ctx_t* client;
    if ((client = client_connect(host, port, username)) == NULL) {
        chat_destroy(msgList);
        LOG_ERR("Client Not Connected");
        return -1;
    }
    chat_push(msgList, CHAT_STATUS, " ", "You joined the Chat");

    ui_t* ui = ui_init(username);
    if (!ui) {
        chat_destroy(msgList);
        client_destroy(client);
        LOG_ERR("ui_init() err");
        return -1;
    }

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = client_getfd(client);
    fds[1].events = POLLIN;

    const char* message;
    msg_t msg;
    ui_refresh(ui, msgList);
    while (g_running) {
        poll(fds, 2, 50);
        if (fds[0].revents & POLLIN) {
            message = ui_handle_key(ui);
            if (message) {
                chat_push(msgList, CHAT_TEXT, username, message);
                client_send_text(client, message);
            }
        }
        if (fds[1].revents & POLLIN) {
            client_recv(client);
            while (client_next_msg(client, &msg)) {
                int cp;
                if (msg.TYPE == MSG_TEXT) {
                    cp = CHAT_TEXT;
                } else if (msg.TYPE == MSG_STATUS) {
                    cp = CHAT_STATUS;
                } else {
                    cp = CHAT_ERR;
                }
                chat_push(msgList, cp, msg.sender, msg.payload);
            }
        }
        ui_refresh(ui, msgList);
    }

    client_disconnect(client);
    ui_destroy(ui);
    client_destroy(client);
    chat_destroy(msgList);
    return 0;
}
