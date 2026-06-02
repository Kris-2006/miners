#include "include/client.h"

#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "include/protocol.h"
struct client_ctx {
    int fd;
    char username[32];
    char recv_buff[2048];
    int recv_len;
};

int client_getfd(client_ctx_t* ctx) { return ctx->fd; }
const char* client_get_username(client_ctx_t* ctx) { return ctx->username; }
static int user_login(client_ctx_t* ctx) {
    char login_status[256];
    msg_t msg_login;
    msg_login.TYPE = MSG_LOGIN;
    strcpy(msg_login.sender, ctx->username);
    msg_login.len = 0;
    int size;
    size = msg_pack(&msg_login, (uint8_t*)login_status, 256);
    send(ctx->fd, login_status, size, 0);
    LOG_INFO("Sent Login..");
    size = recv(ctx->fd, login_status, sizeof(login_status), 0);
    if (size < 0) {
        LOG_ERR("recv-user_login");
        return -1;
    }
    msg_t login_cnf;
    if (msg_unpack((uint8_t*)login_status, sizeof(login_status), &login_cnf) ==
        0) {
        if (strcmp(login_cnf.payload, "login_ok") == 0) {
            LOG_INFO("Login sucessful");
        } else {
            LOG_ERR("Login Unsucessful");
            return -1;
        }
    } else {
        LOG_ERR("Error: Invalid Message from server.");
        return -1;
    }
    return 0;
}

client_ctx_t* client_connect(const char* host, const char* port,
                             const char* username) {
    client_ctx_t* client;
    struct addrinfo hints, *res;
    int status, fd;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(host, port, &hints, &res)) != 0) {
        LOG_ERR("getaddrinfo: %s", gai_strerror(status));
        return NULL;
    }
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1) {
        LOG_ERR("Socket failed");
        return NULL;
    }

    status = connect(fd, (struct sockaddr*)res->ai_addr, res->ai_addrlen);
    if (status == -1) {
        LOG_ERR("connect");
        close(fd);
        return NULL;
    }

    client = (client_ctx_t*)malloc(sizeof(client_ctx_t));
    client->fd = fd;
    client->recv_len = 0;
    strncpy(client->username, username, 32);

    status = user_login(client);
    if (status == -1) {
        LOG_INFO("Retry Again");
        free(client);
        return NULL;
    }

    freeaddrinfo(res);
    return client;
}

bool client_send_text(client_ctx_t* client, const char* msg) {
    msg_t Message;
    Message.TYPE = MSG_TEXT;
    strncpy(Message.sender, client->username, 31);
    Message.sender[31] = '\0';
    strncpy(Message.payload, msg, 255);
    Message.payload[255] = 0;
    Message.len = strnlen(Message.payload, 256);

    char send_buf[256];
    int size;

    size = msg_pack(&Message, (uint8_t*)send_buf, BUFSIZ);
    char* ptr = send_buf;
    while (size > 0) {
        int sent = send(client->fd, ptr, size, 0);
        if (sent == -1) {
            LOG_ERR("TEXT_SEND_ERROR");
            return false;
        }
        ptr += sent;
        size -= sent;
    }
    return true;
}

bool client_disconnect(client_ctx_t* client) {
    char logout_status[256];
    msg_t msg_logout;
    msg_logout.TYPE = MSG_DISCONNECT;
    strncpy(msg_logout.sender, client->username, 32);
    msg_logout.len = 0;
    int size = msg_pack(&msg_logout, (uint8_t*)logout_status, 256);
    if (size < 0) {
        LOG_ERR("Invalid msg pack");
        LOG_INFO("Logout was interrupted.\n");
        return 0;
    }
    if (send(client->fd, logout_status, size, 0) == -1) {
        LOG_ERR("UnSucessful Logout");
        return false;
    }
    LOG_INFO("Sucessfully Logged Out");
    return 1;
}

int client_recv(client_ctx_t* client) {
    int remaining = 2 * MAX_BUFF - client->recv_len;
    int size =
        recv(client->fd, client->recv_buff + client->recv_len, remaining, 0);
    client->recv_len += size;
    return size;
}

bool client_next_msg(client_ctx_t* client, msg_t* msg) {
    if (!client || !msg) return false;

    // check client buf and if there is a messsage parse it into msg. else
    // return false
    //

    if (client->recv_len < 35) return false;

    uint16_t net_len;
    memcpy(&net_len, client->recv_buff + 33, 2);
    uint16_t payload_len = ntohs(net_len);

    int total_len = 35 + payload_len;
    int remaining = client->recv_len - total_len;

    if (remaining < 0) return false;

    int err = msg_unpack((uint8_t*)client->recv_buff, total_len, msg);
    if (err == -1) {
        LOG_ERR("MSG_UNPACK_ERR");
        return false;
    }

    memmove(client->recv_buff, client->recv_buff + total_len, remaining);
    client->recv_len = remaining;

    return true;
}

void client_destroy(client_ctx_t* client) {
    // client_disconnect(client);
    close(client->fd);
    free(client);
}
