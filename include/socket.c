#include "socket.h"

static int build_shakeKey(uint8_t *key)
{
    uint8_t tempKey[SOCKET_SHAKE_KEY_LEN] = {"\0"};
    getRandomString(tempKey, SOCKET_SHAKE_KEY_LEN);
    return base64_encode((const uint8_t *)tempKey, (char *)key, SOCKET_SHAKE_KEY_LEN);
}

static int build_respon_shakeKey(uint8_t *acceptKey, uint32_t acceptKeyLen, uint8_t *respondKey)
{
    char *clientKey;
    char *sha1DataTemp;
    char *sha1Data;
    int i, n;
    const char GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned int GUIDLEN;
    
    if(acceptKey == NULL)
        return 0;
    GUIDLEN = sizeof(GUID);
    clientKey = (char *)calloc(1, sizeof(char)*(acceptKeyLen + GUIDLEN + 10));
    memset(clientKey, 0, (acceptKeyLen + GUIDLEN + 10));

    memcpy(clientKey, acceptKey, acceptKeyLen);
    memcpy(&clientKey[acceptKeyLen], GUID, GUIDLEN);
    clientKey[acceptKeyLen + GUIDLEN] = '\0';

    sha1DataTemp = sha1_hash(clientKey);
    n = strlen(sha1DataTemp);
    sha1Data = (char *)calloc(1, n / 2 + 1);
    memset(sha1Data, 0, n / 2 + 1);
    
    for(i = 0; i < n; i += 2)
        sha1Data[ i / 2 ] = htoi(sha1DataTemp, i, 2);
    
    n = base64_encode((const uint8_t *)sha1Data, (char *)respondKey, (n / 2));

    free(sha1DataTemp);
    free(sha1Data);
    free(clientKey);
    return n;
}

static int match_shakeKey(uint8_t *myKey, uint32_t myKeyLen, uint8_t *acceptKey, uint32_t acceptKeyLen)
{
    int retLen;
    uint8_t tempKey[256] = {"\0"};

    retLen = build_respon_shakeKey(myKey, myKeyLen, tempKey);
}

static void build_header(char *ip, char *port, char *query_path, uint8_t *shakeKey, char *package)
{
    const char header[] = "GET %s HTTP/1.1\r\n"
        "Connection: Upgrade\r\n"
        "Host: %s:%s\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Upgrade: websocket\r\n\r\n";
    sprintf(package, header, query_path, ip, port, shakeKey);
}

int connect_socket(thread *threads, socket_info *socketinfo)
{
    threads->complete ++;

    struct sockaddr_in addr = threads->addr;
    int fd, ret, timeout;
    char buf[RECVBUF] = {"\0"}, send_text[REQUBUF] = "wbench testing", *p;
    /*协议内容*/
    uint8_t shakeBuf[512] = {"\0"}, shakeKey[128] = {"\0"};


    fd = socket(AI_FAMILY, AI_SOCKTYPE, AI_PROTOCOL);
    if (fd < 0) {
        threads->errors.connect ++;
        return -1;
    }
    socketinfo->fd = fd;

    /*非阻塞*/
    ret = fcntl(fd, F_SETFL, 0);
    fcntl(fd, F_SETFL, ret | O_NONBLOCK);

    timeout = 0;
    while (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        if (++timeout > threads->timeout) {
            printf("connect timeout! \n");
            threads->errors.connect ++;
            close(fd);
            return -1;
        }
        delayms(1);
    }

    /*握手key*/
    memset(shakeKey, 0, sizeof(shakeKey));
    build_shakeKey(shakeKey);
    /*协议包*/
    memset(shakeBuf, 0, sizeof(shakeBuf));
    build_header(threads->host, threads->port, "/null", shakeKey, (char *)shakeBuf);
    /*发送协议包*/
    ret = send(fd, shakeBuf, strlen((const char *)shakeBuf), MSG_NOSIGNAL);

    /*握手*/
    while(true) {
        memset(buf, 0, sizeof(buf));
        ret = recv(fd, buf, sizeof(buf), MSG_NOSIGNAL);
        if (ret > 0) {
            /*判断是不是HTTP协议头*/
            if (strncmp((const char *)buf, (const char *)"HTTP", strlen((const char *)"HTTP")) == 0) {
                /*检验握手信号*/
                if ((p = (uint8_t *)strstr((const char*)buf, (const char *)"Sec-WebSocket-Accept: ")) != NULL) {
                    p += strlen((const char *)"Sec-WebSocket-Accept: "); 
                    sscanf((const char *)p, "%s\r\n", p);
                    /*握手成功*/
                    if (match_shakeKey(shakeKey, strlen((const char*)shakeKey), p, strlen((const char *)p)) == 0) {
                        return fd;
                    } else {
                        /*握手不对，重新发送协议包*/
                        ret = send(fd, shakeBuf, strlen((const char *)shakeBuf), MSG_NOSIGNAL);
                    }
                } else {
                    ret = send(fd, shakeBuf, strlen((const char *)shakeBuf), MSG_NOSIGNAL);
                }
            }
        }
        if (++timeout > threads->timeout) {
            printf("shake timeout! \n");
            threads->errors.connect ++;
            close(fd);
            return -1;
        }
        delayms(1);
    }
    if (threads->params[0] != '\0') {
        strcpy(send_text, threads->params); 
    }


    send(fd, send_text, strlen(send_text) + 1, 0);
    recv(fd, buf, RECVBUF, 0);

    close(fd);

    strcpy(socketinfo->buf, buf);

    threads->requests ++;
    threads->bytes += strlen(buf);

    return fd;
}
