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
    uint32_t GUIDLEN;

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

    if(retLen != acceptKeyLen) {
        return -1;
    } else if(strcmp((const char *)tempKey, (const char *)acceptKey) != 0) {
        return -1;
    }

    return 0;
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

static void build_respon_header(uint8_t *acceptKey, uint32_t acceptKeyLen, char *package)
{
    const char header[] = "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Server: Microsoft-HTTPAPI/2.0\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "%s\r\n\r\n";

    time_t now;
    struct tm *tm_now;
    char timeStr[256] = {0};
    uint8_t respondShakeKey[256] = {0};
    /*构建回应的握手key*/
    build_respon_shakeKey(acceptKey, acceptKeyLen, respondShakeKey);   
    /*构建回应时间字符串*/
    time(&now);
    tm_now = localtime(&now);
    /*时间打包待续  格式如 "Date: Tue, 20 Jun 2017 08:50:41 CST\r\n"*/
    strftime(timeStr, sizeof(timeStr), "Date: %a, %d %b %Y %T %Z", tm_now);
    /*组成回复信息*/
    sprintf(package, header, respondShakeKey, timeStr);
}
/*******************************************************************************
 * 名称: w_enPackage
 * 功能: websocket数据收发阶段的数据打包, 通常client发server的数据都要isMask(掩码)处理, 反之server到client却不用
 * 形参: *data：准备发出的数据
 *          dataLen : 长度
 *        *package : 打包后存储地址
 *        packageMaxLen : 存储地址可用长度
 *          isMask : 是否使用掩码     1要   0 不要
 *          type : 数据类型, 由打包后第一个字节决定, 这里默认是数据传输, 即0x81
 * 返回: 打包后的长度(会比原数据长2~16个字节不等)      <=0 打包失败 
 * 说明: 无
 ******************************************************************************/
static int w_enPackage(uint8_t *data, uint32_t dataLen, uint8_t *package, uint32_t packageMaxLen, 
        bool isMask, w_com_type type)
{
    /*掩码*/
    uint8_t maskKey[4] = {0};
    uint8_t temp1, temp2;
    int count;
    uint32_t i, len = 0;

    if(packageMaxLen < 2)
        return -1;

    if(type == WCT_MINDATA)
        *package++ = 0x00;
    else if(type == WCT_TXTDATA)
        *package++ = 0x81;
    else if(type == WCT_BINDATA)
        *package++ = 0x82;
    else if(type == WCT_DISCONN)
        *package++ = 0x88;
    else if(type == WCT_PING)
        *package++ = 0x89;
    else if(type == WCT_PONG)
        *package++ = 0x8A;
    else
        return -1;

    if(isMask)
        *package = 0x80;
    len += 1;

    if(dataLen < 126) {
        *package++ |= (dataLen&0x7F);
        len += 1;
    } else if(dataLen < 65536) {
        if(packageMaxLen < 4)
            return -1;
        *package++ |= 0x7E;
        *package++ = (char)((dataLen >> 8) & 0xFF);
        *package++ = (uint8_t)((dataLen >> 0) & 0xFF);
        len += 3;
    } else if(dataLen < 0xFFFFFFFF) {
        if(packageMaxLen < 10)
            return -1;
        *package++ |= 0x7F;
        /*(char)((dataLen >> 56) & 0xFF);   数据长度变量是 uint32_t dataLen, 暂时没有那么多数据*/
        *package++ = 0; 
        /*(char)((dataLen >> 48) & 0xFF);*/
        *package++ = 0;
        /*(char)((dataLen >> 40) & 0xFF);*/
        *package++ = 0;
        /*(char)((dataLen >> 32) & 0xFF);*/
        *package++ = 0;
        /*到这里就够传4GB数据了*/
        *package++ = (char)((dataLen >> 24) & 0xFF);
        *package++ = (char)((dataLen >> 16) & 0xFF);
        *package++ = (char)((dataLen >> 8) & 0xFF);
        *package++ = (char)((dataLen >> 0) & 0xFF);
        len += 9;
    }
    /*数据使用掩码时, 使用异或解码, maskKey[4]依次和数据异或运算, 逻辑如下*/
    if(isMask) {
        if(packageMaxLen < len + dataLen + 4)
            return -1;
        
        /*随机生成掩码*/
        getRandomString(maskKey, sizeof(maskKey));
        *package++ = maskKey[0];
        *package++ = maskKey[1];
        *package++ = maskKey[2];
        *package++ = maskKey[3];
        len += 4;
        for(i = 0, count = 0; i < dataLen; i++) {
            temp1 = maskKey[count];
            temp2 = data[i];
            /*异或运算后得到数据*/
            *package++ = (char)(((~temp1)&temp2) | (temp1&(~temp2)));
            count += 1;
            /*maskKey[4]循环使用*/
            if(count >= sizeof(maskKey))
                count = 0;
        }
        len += i;
        *package = '\0';
    } else {
        /*数据没使用掩码, 直接复制数据段*/
        if(packageMaxLen < len + dataLen)
            return -1;
        memcpy(package, data, dataLen);
        package[dataLen] = '\0';
        len += dataLen;
    }
    
    return len;
}
/*******************************************************************************
 * 名称: webSocket_dePackage
 * 功能: websocket数据收发阶段的数据解包, 通常client发server的数据都要isMask(掩码)处理, 反之server到client却不用
 * 形参: *data：解包的数据
 *          dataLen : 长度
 *        *package : 解包后存储地址
 *        packageMaxLen : 存储地址可用长度
 *        *packageLen : 解包所得长度
 * 返回: 解包识别的数据类型 如 : txt数据, bin数据, ping, pong等
 * 说明: 无
 ******************************************************************************/
int w_dePackage(uint8_t *data, uint32_t dataLen, uint8_t *package, uint32_t packageMaxLen, 
        uint32_t *packageLen)
{
    /*掩码*/
    uint8_t maskKey[4] = {0};
    uint8_t temp1, temp2;
    char Mask = 0, type;
    int count, ret;
    uint32_t i, len = 0, dataStart = 2;
    if(dataLen < 2)
        return -1;

    type = data[0]&0x0F;

    if((data[0]&0x80) == 0x80) {
        if(type == 0x01) 
            ret = WCT_TXTDATA;
        else if(type == 0x02) 
            ret = WCT_BINDATA;
        else if(type == 0x08) 
            ret = WCT_DISCONN;
        else if(type == 0x09) 
            ret = WCT_PING;
        else if(type == 0x0A) 
            ret = WCT_PONG;
        else 
            return WCT_ERR;
    } else if(type == 0x00) {
        ret = WCT_MINDATA;
    } else {
        return WCT_ERR;
    }

    if((data[1] & 0x80) == 0x80) {
        Mask = 1;
        count = 4;
    } else {
        Mask = 0;
        count = 0;
    }

    len = data[1] & 0x7F;

    if(len == 126) {
        if(dataLen < 4)
            return WCT_ERR;
        len = data[2];
        len = (len << 8) + data[3];
        if(dataLen < len + 4 + count)
            return WCT_ERR;
        if(Mask) {
            maskKey[0] = data[4];
            maskKey[1] = data[5];
            maskKey[2] = data[6];
            maskKey[3] = data[7];
            dataStart = 8;
        } else {
            dataStart = 4;
        }
    } else if(len == 127) {
        if(dataLen < 10)
            return WCT_ERR;

        /*使用8个字节存储长度时, 前4位必须为0, 装不下那么多数据...*/
        if(data[2] != 0 || data[3] != 0 || data[4] != 0 || data[5] != 0)
            return WCT_ERR;
        len = data[6];
        len = (len << 8) + data[7];
        len = (len << 8) + data[8];
        len = (len << 8) + data[9];
        if(dataLen < len + 10 + count)
            return WCT_ERR;

        if(Mask) {
            maskKey[0] = data[10];
            maskKey[1] = data[11];
            maskKey[2] = data[12];
            maskKey[3] = data[13];
            dataStart = 14;
        } else {
            dataStart = 10;
        }
    } else {
        if(dataLen < len + 2 + count)
            return WCT_ERR;

        if(Mask) {
            maskKey[0] = data[2];
            maskKey[1] = data[3];
            maskKey[2] = data[4];
            maskKey[3] = data[5];
            dataStart = 6;
        } else {
            dataStart = 2;
        }
    }

    if(dataLen < len + dataStart)
        return WCT_ERR;

    if(packageMaxLen < len + 1)
        return WCT_ERR;

    /*解包数据使用掩码时, 使用异或解码, maskKey[4]依次和数据异或运算, 逻辑如下*/
    if(Mask) {
        for(i = 0, count = 0; i < len; i++) {
            temp1 = maskKey[count];
            temp2 = data[i + dataStart];
            /*异或运算后得到数据*/
            *package++ =  (char)(((~temp1)&temp2) | (temp1&(~temp2)));
            count += 1;
            /*maskKey[4]循环使用*/
            if(count >= sizeof(maskKey))
                count = 0;
        }
        *package = '\0';
    } else {
        /*解包数据没使用掩码, 直接复制数据段*/
        memcpy(package, &data[dataStart], len);
        package[len] = '\0';
    }
    *packageLen = len;
    return ret;
}

/*******************************************************************************
 * 名称: w_serverToClient
 * 功能: 服务器回复客户端的连接请求, 以建立websocket连接
 * 形参: fd：连接句柄
 *          *recvBuf : 接收到来自客户端的数据(内含http连接请求)
 *              bufLen : 
 * 返回: =0 建立websocket连接成功        <0 建立websocket连接失败
 * 说明: 无
 ******************************************************************************/
int w_serverToClient(int fd, char *recvBuf, unsigned int bufLen)
{
    char *p;
    int ret;
    char recvShakeKey[512], respondPackage[1024];
    
    if((p = strstr(recvBuf, "Sec-WebSocket-Key: ")) == NULL)
        return -1;

    p += strlen("Sec-WebSocket-Key: ");

    memset(recvShakeKey, 0, sizeof(recvShakeKey));
    /*取得握手key*/
    sscanf(p, "%s", recvShakeKey);
    ret = strlen(recvShakeKey);
    if(ret < 1) {
        return -1;
    }
    memset(respondPackage, 0, sizeof(respondPackage));
    build_respon_header(recvShakeKey, ret, respondPackage);

    return send(fd, respondPackage, strlen(respondPackage), MSG_NOSIGNAL);
}

static int w_clientToServer(thread *threads)
{
    struct sockaddr_in addr = threads->addr;
    int fd, ret, timeout;
    char buf[RECVBUF] = {"\0"}, *p;
    /*协议内容*/
    uint8_t shakeBuf[512] = {"\0"}, shakeKey[128] = {"\0"};

    fd = socket(AI_FAMILY, AI_SOCKTYPE, AI_PROTOCOL);
    if (fd < 0) {
        return -1;
    }

    /*非阻塞*/
    ret = fcntl(fd, F_SETFL, 0);
    fcntl(fd, F_SETFL, ret | O_NONBLOCK);

    timeout = 0;
    while (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        if (++timeout > threads->timeout) {
            printf("connect timeout! \n");
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

        return -1;
    }
}

/*******************************************************************************
 * 名称: w_send
 * 功能: websocket数据基本打包和发送
 * 形参: fd：连接句柄
 *          *data : 数据
 *          dataLen : 长度
 *          mod : 数据是否使用掩码, 客户端到服务器必须使用掩码模式
 *          type : 数据要要以什么识别头类型发送(txt, bin, ping, pong ...)
 * 返回: 调用send的返回
 * 说明: 无
 ******************************************************************************/
static int w_send(int fd, uint8_t *data, uint32_t dataLen, bool mod, w_com_type type)
{
    uint8_t *webSocketPackage;
    uint32_t retLen, ret;

    /*websocket数据打包*/
    webSocketPackage = (uint8_t *)calloc(1, sizeof(char)*(dataLen + 128));
    memset(webSocketPackage, 0, (dataLen + 128));
    retLen = w_enPackage(data, dataLen, webSocketPackage, (dataLen + 128), mod, type);
    ret = send(fd, webSocketPackage, retLen, MSG_NOSIGNAL);
    free(webSocketPackage);
    return ret;
}
/*******************************************************************************
 * 名称: w_recv
 * 功能: websocket数据接收和基本解包
 * 形参: fd：连接句柄
 *          *data : 数据接收地址
 *          dataMaxLen : 接收区可用最大长度
 * 返回: <= 0 没有收到有效数据        > 0 成功接收并解包数据
 * 说明: 无
 ******************************************************************************/
static int w_recv(int fd, uint8_t *data, uint32_t dataMaxLen)
{
    uint8_t *webSocketPackage, *recvBuf;
    int ret, ret2 = 0;
    uint32_t retLen = 0;

    recvBuf = (uint8_t *)calloc(1, sizeof(char)*dataMaxLen);
    memset(recvBuf, 0, dataMaxLen);
    ret = recv(fd, recvBuf, dataMaxLen, MSG_NOSIGNAL);
    if(ret > 0) {
        if(strncmp(recvBuf, "GET", 3) == 0) {
            ret2 = w_serverToClient(fd, recvBuf, ret);
            free(recvBuf);
            if(ret2 < 0) {
                memset(data, 0, dataMaxLen);
                strcpy(data, "connect false !\r\n");
                return strlen("connect false !\r\n");
            }
            memset(data, 0, dataMaxLen);
            strcpy(data, "connect ...\r\n");
            return strlen("connect ...\r\n");
        }

        /*websocket数据打包*/
        webSocketPackage = (uint8_t *)calloc(1, sizeof(char)*(ret + 128));
        memset(webSocketPackage, 0, (ret + 128));
        ret2 = w_dePackage(recvBuf, ret, webSocketPackage, (ret + 128), &retLen);
        /*解析为ping包, 自动回pong*/
        if(ret2 == WCT_PING && retLen > 0) {
            w_send(fd, webSocketPackage, retLen, true, WCT_PONG);
            /*显示数据*/
            printf("webSocket_recv : PING %d\r\n%s\r\n" , retLen, webSocketPackage); 
            free(recvBuf);
            free(webSocketPackage);
            return WCT_NULL;
        } else if(retLen > 0 && (ret2 == WCT_TXTDATA || ret2 == WCT_BINDATA || ret2 == WCT_MINDATA)) {
            /*解析为数据包*/
            /*把解析得到的数据复制出去*/
            memcpy(data, webSocketPackage, retLen);
            free(recvBuf);
            free(webSocketPackage);
            return retLen;
        }
        free(recvBuf);
        free(webSocketPackage);
        return -ret;
    } else {
        free(recvBuf);
        return ret;
    }
}

int connect_socket(thread *threads, socket_info *socketinfo)
{
    threads->complete ++;

    int fd, ret, timeout;
    char buf[RECVBUF] = {"\0"}, send_text[REQUBUF] = "wbench testing", *p;

    fd = w_clientToServer(threads);

    socketinfo->fd = fd;

    if (threads->params[0] != '\0') {
        strcpy(send_text, threads->params); 
    }

    /*send(fd, send_text, strlen(send_text) + 1, 0);*/
    /*recv(fd, buf, RECVBUF, 0);*/
    ret = w_send(fd, send_text, strlen(send_text), true, WCT_TXTDATA);
    while (true) {
        memset(buf, 0, sizeof(buf));
        ret = w_recv(fd, buf, sizeof(buf));
        if (++timeout > threads->timeout) {
            break;
        }
        delayms(1);
    }

    close(fd);

    strcpy(socketinfo->buf, buf);

    threads->requests ++;
    threads->bytes += strlen(buf);

    return fd;
}
