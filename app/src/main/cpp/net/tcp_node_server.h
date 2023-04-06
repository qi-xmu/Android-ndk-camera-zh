//
// Created by 36014 on 2023/4/5.
//

#ifndef TCAMERA_TCP_NODE_SERVER_H
#define TCAMERA_TCP_NODE_SERVER_H

#include <netinet/in.h>


class TcpNodeServer {
public:

    TcpNodeServer(uint32_t bsize = 1024, uint32_t port = 8080);

    ~TcpNodeServer();

    void wait_for_client();

    bool i32_send(const int32_t num);

    int32_t str_send(const char *str);

    int32_t bin_send(const uint8_t *buf, uint32_t size);


    // 状态变量
    int valid;

private:
    int _server_fd;
    int _client_fd;
    struct sockaddr_in _server_address;
    struct sockaddr_in _client_address;

    uint8_t *_buffer;

    int32_t buffer_send(const uint8_t *buf, uint64_t blen);
};

#endif //TCAMERA_TCP_NODE_SERVER_H
