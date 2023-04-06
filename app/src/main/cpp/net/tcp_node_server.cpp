//
// Created by 36014 on 2023/4/5.
//

#include "tcp_node_server.h"
#include "../native-log.h"
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "192.168.8.2"
#define SERVER_PORT 8080

void test_socket_client() {
    LOG_INFO("Start test_socket_client");
    int sock_fd;
    int res;
    struct sockaddr_in server_addr{};
    memset(&server_addr, 0, sizeof(server_addr));

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        LOG_ERR("error in sock_fd");
        return;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    res = connect(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (res == -1) {
        LOG_ERR("error in connect");
        return;
    }

    LOG_INFO("Connect to %s:%d", SERVER_IP, SERVER_PORT);

    char hello[] = "hello world";

    res = write(sock_fd, hello, sizeof(hello));
    if (res < 0) {
        LOG_ERR("error in write");
    }
    char buffer[100];
    memset(buffer, 0, sizeof(buffer));
    res = read(sock_fd, buffer, sizeof(buffer));
    if (res < 0) {
        LOG_ERR("error in read");
    }
    buffer[res] = 0;
    LOG_INFO("Received %s", buffer);

    if (sock_fd > 0) {
//        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
    }
}

TcpNodeServer::TcpNodeServer(uint32_t bsize, uint32_t port) {
    valid = false;

    memset(&_server_address, 0, sizeof(_server_address));
    _server_address.sin_family = AF_INET;
    _server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    _server_address.sin_port = htons(port);

    _buffer = (uint8_t *) malloc(bsize);
    memset(_buffer, 0, bsize);
    valid = true;
}

void TcpNodeServer::wait_for_client() {
    int res;
    _server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_server_fd < 0) {
        LOG_ERR("error in sock_fd");
        return;
    }
    res = bind(_server_fd, (struct sockaddr *) &_server_address, sizeof(_server_address));
    if (res) {
        LOG_ERR("error in bind %d", res);
        return;
    }
    if (listen(_server_fd, 1)) {
        LOG_ERR("error in listen");
        return;
    }
    socklen_t _client_socket_len = sizeof(_client_address);
    _client_fd = accept(_server_fd, (struct sockaddr *) &_client_address, &_client_socket_len);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &_client_address.sin_addr, ip, INET_ADDRSTRLEN);
    LOG_INFO("%s Connected", ip);
    if (str_send("start.") <= 0) {
        LOG_ERR("str_send error");
    }
}

int32_t TcpNodeServer::buffer_send(const uint8_t *buf, uint64_t blen) {
    int32_t ret = -1;
    if (_client_fd > 0) {
        ret = send(_client_fd, buf, blen, MSG_WAITALL);
        if (ret <= 0) {
            valid = false;
        }
    }
    return ret;
}

enum DATA_TYPE {
    TCP_NODE_STRING = 0,
    TCP_NODE_BINARY = 1,
};

int32_t TcpNodeServer::str_send(const char *str) {
    int32_t ret = -1;
    if (valid) {
        auto len = (int32_t) strlen(str);
        i32_send(len);
        ret = buffer_send((const uint8_t *) str, len);
        i32_send(TCP_NODE_STRING);
    }
    return ret;
}

TcpNodeServer::~TcpNodeServer() {
    if (_client_fd) {
        close(_client_fd);
    }
    if (_server_fd) {
        close(_server_fd);
    }
    if (_buffer) {
        free(_buffer);
    }
}

bool TcpNodeServer::i32_send(const int32_t num) {
    return buffer_send((uint8_t *) &num, sizeof(num));
}

int32_t TcpNodeServer::bin_send(const uint8_t *buf, uint32_t size) {
    int32_t ret = -1;
    if (valid) {
        i32_send((int) size);
        ret = buffer_send(buf, size);
        i32_send(TCP_NODE_BINARY);
    }
    return ret;
}



