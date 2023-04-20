//
// Created by 36014 on 2023/4/6.
//

#ifndef TCAMERA_TCP_NODE_CLIENT_H
#define TCAMERA_TCP_NODE_CLIENT_H

#include <netinet/in.h>

/**
 * 数据发送格式：
 * 长度 + 类型 \n
 * 数据体 \n
 */

class TcpNodeClient {
public:

    TcpNodeClient(const char *address, int32_t port);

    ~TcpNodeClient();

    bool connect_server();

    bool bin_send(const uint8_t *bin, int32_t size, int32_t type = 1);

    bool str_send(const char *str, int32_t size = -1);

    bool getValid() const;


private:
    bool _valid;
    int _server_fd;
    struct sockaddr_in _server_address;

    int32_t buffer_send(const char *buffer, int32_t size);
};


#endif //TCAMERA_TCP_NODE_CLIENT_H
