//
// Created by 36014 on 2023/4/6.
//

#include "tcp_node_client.h"
#include "../native-log.h"
#include <cstring>
#include <arpa/inet.h>
#include <cstdlib>
#include <sys/socket.h>

TcpNodeClient::TcpNodeClient(const char *address, int32_t port) :
        _server_fd(0),
        _valid(false) {
    memset(&_server_address, 0, sizeof(_server_address));
    _server_address.sin_family = AF_INET;
    _server_address.sin_addr.s_addr = inet_addr(address);
    _server_address.sin_port = htons(port);
}

bool TcpNodeClient::connect_server() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd > 0) {
        int res = connect(
                _server_fd,
                (struct sockaddr *) &_server_address,
                sizeof(_server_address)
        );
        _valid = true;
        if (res < 0) {
            _valid = false;
            throw "connect to server error";
        }
    } else {
        throw "_server_fd is error";
    }
    return _valid;
}

#include <cassert>

int32_t TcpNodeClient::buffer_send(const char *buffer, int32_t size) {
    int send_bytes = send(_server_fd, buffer, size, MSG_WAITALL);
    if (send_bytes != size) {
        throw send_bytes;
    }
    return send_bytes;
}


#include <unistd.h>

TcpNodeClient::~TcpNodeClient() {
    if (_server_fd > 0) {
        close(_server_fd);
        _server_fd = -1;
    }
}

bool TcpNodeClient::bin_send(const uint8_t *bin, int32_t size, int32_t type) {
    // 发送类型 binary 1
    int ret = false;
    try {
        buffer_send((char *) &size, sizeof(size));
        buffer_send((char *) &type, sizeof(type));
        char *send_str = (char *) (bin);
        while (true) {
            try {
                buffer_send(send_str, size);
                break;
            } catch (const int bytes) {
                if (bytes <= 0) {
                    return ret; // send interrupt
                }
                send_str = send_str + bytes;
                size -= bytes;
            }
        }
        ret = true;
    } catch (const int bytes) {
        LOG_ERR("Send error.");
        _valid = false;
    }
    return ret;
}


bool TcpNodeClient::str_send(const char *str, int32_t size) {
    // 发送类型 string 0
    if (size < 0) {
        size = (int) strlen(str);
    }
    return bin_send((uint8_t *) str, size, 0);
}

bool TcpNodeClient::getValid() const {
    return _valid;
}




