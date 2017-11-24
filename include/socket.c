#include "socket.h"

static int build_shake_key(uint8_t *key)
{
    uint8_t temp_key[SOCKET_SHAKE_KEY_LEN] = {"\0"};
    get_random_string(temp_key, SOCKET_SHAKE_KEY_LEN);
    return base64_encode((const uint8_t *)temp_key, (char *)key, SOCKET_SHAKE_KEY_LEN);
}

static int build_respon_shake_key(uint8_t *accept_key, uint32_t accept_key_len, uint8_t *respond_key)
{
    char *client_key;
    char *sha1_data_temp;
    char *sha1_data;
    int i, n;
    const char guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    uint32_t guidlen;

    if(accept_key == NULL)
        return 0;

    guidlen = sizeof(guid);
    client_key = (char *)calloc(1, sizeof(char)*(accept_key_len + guidlen + 10));
    memset(client_key, 0, (accept_key_len + guidlen + 10));

    memcpy(client_key, accept_key, accept_key_len);
    memcpy(&client_key[accept_key_len], guid, guidlen);
    client_key[accept_key_len + guidlen] = '\0';

    sha1_data_temp = sha1_hash(client_key);
    n = strlen(sha1_data_temp);
    sha1_data = (char *)calloc(1, n / 2 + 1);
    memset(sha1_data, 0, n / 2 + 1);

    for(i = 0; i < n; i += 2)
        sha1_data[ i / 2 ] = htoi(sha1_data_temp, i, 2);

    n = base64_encode((const uint8_t *)sha1_data, (char *)respond_key, (n / 2));

    free(sha1_data_temp);
    free(sha1_data);
    free(client_key);
    return n;
}

static int match_shake_key(uint8_t *my_key, uint32_t my_key_len, uint8_t *accept_key, 
        uint32_t accept_key_len)
{
    int ret_len;
    uint8_t temp_key[256] = {"\0"};

    ret_len = build_respon_shake_key(my_key, my_key_len, temp_key);

    if(ret_len != accept_key_len) {
        return -1;
    } else if(strcmp((const char *)temp_key, (const char *)accept_key) != 0) {
        return -1;
    }

    return 0;
}

static void build_header(char *ip, char *port, char *query_path, uint8_t *shake_key, char *package)
{
    const char header[] = "GET %s HTTP/1.1\r\n"
        "Connection: Upgrade\r\n"
        "Host: %s:%s\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Upgrade: websocket\r\n\r\n";
    sprintf(package, header, query_path, ip, port, shake_key);
}

static void build_respon_header(uint8_t *accept_key, uint32_t accept_key_len, char *package)
{
    const char header[] = "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Server: Microsoft-HTTPAPI/2.0\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "%s\r\n\r\n";

    time_t now;
    struct tm *tm_now;
    char time_str[256] = {0};
    uint8_t respond_shake_key[256] = {0};
    /*构建回应的握手key*/
    build_respon_shake_key(accept_key, accept_key_len, respond_shake_key);   
    /*构建回应时间字符串*/
    time(&now);
    tm_now = localtime(&now);
    /*时间打包待续  格式如 "Date: Tue, 20 Jun 2017 08:50:41 CST\r\n"*/
    strftime(time_str, sizeof(time_str), "Date: %a, %d %b %Y %T %Z", tm_now);
    /*组成回复信息*/
    sprintf(package, header, respond_shake_key, time_str);
}

/**
 * websocket数据收发阶段的数据打包, 通常client发server的数据都要isMask(掩码)处理, 
 * 反之server到client却不用
 *
 * @param uint8_t       *data               准备发出的数据
 * @param uint32_t      data_len            长度
 * @param uint8_t       *package            打包后存储地址
 * @param uint32_t      package_max_len     存储地址可用长度
 * @param bool          is_mask             是否使用掩码     1要   0 不要
 * @param w_com_type    type                数据类型, 由打包后第一个字节决定, 这里默认是数据传输, 即0x81
 *
 * @return uint32_t     打包后的长度(会比原数据长2~16个字节不等)      <=0 打包失败
 *
 */
static int w_enpackage(uint8_t *data, uint32_t data_len, uint8_t *package, uint32_t package_max_len, 
        bool is_mask, w_com_type type)
{
    /*掩码*/
    uint8_t mask_key[4] = {0};
    uint8_t temp1, temp2;
    int count;
    uint32_t i, len = 0;

    if(package_max_len < 2)
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

    if(is_mask)
        *package = 0x80;
    len += 1;

    if(data_len < 126) {
        *package++ |= (data_len&0x7F);
        len += 1;
    } else if(data_len < 65536) {
        if(package_max_len < 4)
            return -1;
        *package++ |= 0x7E;
        *package++ = (char)((data_len >> 8) & 0xFF);
        *package++ = (uint8_t)((data_len >> 0) & 0xFF);
        len += 3;
    } else if(data_len < 0xFFFFFFFF) {
        if(package_max_len < 10)
            return -1;
        *package++ |= 0x7F;
        /*(char)((data_len >> 56) & 0xFF);   数据长度变量是 uint32_t data_len, 暂时没有那么多数据*/
        *package++ = 0; 
        /*(char)((data_len >> 48) & 0xFF);*/
        *package++ = 0;
        /*(char)((data_len >> 40) & 0xFF);*/
        *package++ = 0;
        /*(char)((data_len >> 32) & 0xFF);*/
        *package++ = 0;
        /*到这里就够传4GB数据了*/
        *package++ = (char)((data_len >> 24) & 0xFF);
        *package++ = (char)((data_len >> 16) & 0xFF);
        *package++ = (char)((data_len >> 8) & 0xFF);
        *package++ = (char)((data_len >> 0) & 0xFF);
        len += 9;
    }
    /*数据使用掩码时, 使用异或解码, maskKey[4]依次和数据异或运算, 逻辑如下*/
    if(is_mask) {
        if(package_max_len < len + data_len + 4)
            return -1;
        
        /*随机生成掩码*/
        get_random_string(mask_key, sizeof(mask_key));
        *package++ = mask_key[0];
        *package++ = mask_key[1];
        *package++ = mask_key[2];
        *package++ = mask_key[3];
        len += 4;
        for(i = 0, count = 0; i < data_len; i++) {
            temp1 = mask_key[count];
            temp2 = data[i];
            /*异或运算后得到数据*/
            *package++ = (char)(((~temp1)&temp2) | (temp1&(~temp2)));
            count += 1;
            /*maskKey[4]循环使用*/
            if(count >= sizeof(mask_key))
                count = 0;
        }
        len += i;
        *package = '\0';
    } else {
        /*数据没使用掩码, 直接复制数据段*/
        if(package_max_len < len + data_len)
            return -1;
        memcpy(package, data, data_len);
        package[data_len] = '\0';
        len += data_len;
    }
    
    return len;
}

/**
 * websocket数据收发阶段的数据解包, 通常client发server的数据都要isMask(掩码)处理, 
 * 反之server到client却不用
 *
 * @param uint8_t   *data           解包的数据
 * @param uint32_t  data_len        长度
 * @param uint8_t   *package        解包后存储地址
 * @param uint32_t  package_max_len 存储地址可用长度
 * @param uint32_t  *package_len    解包所得长度
 *
 * @return int      解包识别的数据类型 如 : txt数据, bin数据, ping, pong等
 *
 */
int w_depackage(uint8_t *data, uint32_t data_len, uint8_t *package, uint32_t package_max_len, 
        uint32_t *package_len)
{
    /*掩码*/
    uint8_t mask_key[4] = {0};
    uint8_t temp1, temp2;
    char mask = 0, type;
    int count, ret;
    uint32_t i, len = 0, data_start = 2;
    if(data_len < 2)
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
        mask = 1;
        count = 4;
    } else {
        mask = 0;
        count = 0;
    }

    len = data[1] & 0x7F;

    if(len == 126) {
        if(data_len < 4)
            return WCT_ERR;
        len = data[2];
        len = (len << 8) + data[3];
        if(data_len < len + 4 + count)
            return WCT_ERR;
        if(mask) {
            mask_key[0] = data[4];
            mask_key[1] = data[5];
            mask_key[2] = data[6];
            mask_key[3] = data[7];
            data_start = 8;
        } else {
            data_start = 4;
        }
    } else if(len == 127) {
        if(data_len < 10)
            return WCT_ERR;

        /*使用8个字节存储长度时, 前4位必须为0, 装不下那么多数据...*/
        if(data[2] != 0 || data[3] != 0 || data[4] != 0 || data[5] != 0)
            return WCT_ERR;
        len = data[6];
        len = (len << 8) + data[7];
        len = (len << 8) + data[8];
        len = (len << 8) + data[9];
        if(data_len < len + 10 + count)
            return WCT_ERR;

        if(mask) {
            mask_key[0] = data[10];
            mask_key[1] = data[11];
            mask_key[2] = data[12];
            mask_key[3] = data[13];
            data_start = 14;
        } else {
            data_start = 10;
        }
    } else {
        if(data_len < len + 2 + count)
            return WCT_ERR;

        if(mask) {
            mask_key[0] = data[2];
            mask_key[1] = data[3];
            mask_key[2] = data[4];
            mask_key[3] = data[5];
            data_start = 6;
        } else {
            data_start = 2;
        }
    }

    if(data_len < len + data_start)
        return WCT_ERR;

    if(package_max_len < len + 1)
        return WCT_ERR;

    /*解包数据使用掩码时, 使用异或解码, maskKey[4]依次和数据异或运算, 逻辑如下*/
    if(mask) {
        for(i = 0, count = 0; i < len; i++) {
            temp1 = mask_key[count];
            temp2 = data[i + data_start];
            /*异或运算后得到数据*/
            *package++ =  (char)(((~temp1)&temp2) | (temp1&(~temp2)));
            count += 1;
            /*mask_key[4]循环使用*/
            if(count >= sizeof(mask_key))
                count = 0;
        }
        *package = '\0';
    } else {
        /*解包数据没使用掩码, 直接复制数据段*/
        memcpy(package, &data[data_start], len);
        package[len] = '\0';
    }
    *package_len = len;
    return ret;
}

/**
 * 服务器回复客户端的连接请求, 以建立websocket连接
 *
 * @param int       fd          连接句柄
 * @param char      *recv_buf   接收到来自客户端的数据(内含http连接请求)
 * @param uint32_t  buf_len : 
 *
 * @return int      =0 建立websocket连接成功        <0 建立websocket连接失败
 *
 */
int w_server_to_client(int fd, char *recv_buf, uint32_t buf_len)
{
    char *p;
    int ret;
    char recv_shake_key[512], respond_package[1024];
    
    if((p = strstr(recv_buf, "Sec-WebSocket-Key: ")) == NULL)
        return -1;

    p += strlen("Sec-WebSocket-Key: ");

    memset(recv_shake_key, 0, sizeof(recv_shake_key));
    /*取得握手key*/
    sscanf(p, "%s", recv_shake_key);
    ret = strlen(recv_shake_key);
    if(ret < 1) {
        return -1;
    }
    memset(respond_package, 0, sizeof(respond_package));
    build_respon_header(recv_shake_key, ret, respond_package);

    return send(fd, respond_package, strlen(respond_package), MSG_NOSIGNAL);
}

/*客户端连接服务端握手处理*/
static int w_client_to_server(thread *threads)
{
    struct sockaddr_in addr = threads->addr;
    int fd, ret, timeout;
    char buf[RECVBUF] = {"\0"}, *p;
    /*协议内容*/
    uint8_t shake_buf[512] = {"\0"}, shake_key[128] = {"\0"};

    fd = socket(AI_FAMILY, AI_SOCKTYPE, AI_PROTOCOL);
    if (fd < 0) {
        threads->errors.connect ++;
        printf("socket create faild \n");
        return fd;
    }

    /*非阻塞*/
    ret = fcntl(fd, F_SETFL, 0);
    fcntl(fd, F_SETFL, ret | O_NONBLOCK);

    timeout = 0;
    while (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        if (++timeout > threads->timeout) {
            threads->errors.connect ++;
            /*printf("connect timeout \n");*/
            close(fd);
            return -1;
        }
        delayms(1);
    }

    /*握手key*/
    memset(shake_key, 0, sizeof(shake_key));
    build_shake_key(shake_key);
    /*协议包*/
    memset(shake_buf, 0, sizeof(shake_buf));
    build_header(threads->host, threads->port, threads->uri, shake_key, (char *)shake_buf);
    /*发送协议包*/
    ret = send(fd, shake_buf, strlen((const char *)shake_buf), MSG_NOSIGNAL);

    /*握手*/
    timeout = 0;
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
                    if (match_shake_key(shake_key, strlen((const char*)shake_key), p, 
                                strlen((const char *)p)) == 0) {
                        return fd;
                    } else {
                        /*握手不对，重新发送协议包*/
                        ret = send(fd, shake_buf, strlen((const char *)shake_buf), MSG_NOSIGNAL);
                    }
                } else {
                    ret = send(fd, shake_buf, strlen((const char *)shake_buf), MSG_NOSIGNAL);
                }
            }
        }
        if (++timeout > threads->timeout) {
            /*printf("shake timeout \n");*/
            threads->errors.connect ++;
            close(fd);
            return -1;
        }
        delayms(1);
    }
    return -1;
}

/**
 * websocket数据基本打包和发送
 *
 * @param int           fd          连接句柄
 * @param uint8_t       *data       数据
 * @param uint32_t      data_len    长度
 * @param bool          mod         数据是否使用掩码, 客户端到服务器必须使用掩码模式
 * @param w_com_type    type        数据要要以什么识别头类型发送(txt, bin, ping, pong ...)
 *
 * @return uint32_t     调用send的返回
 *
 */
static int w_send(int fd, uint8_t *data, uint32_t data_len, bool mod, w_com_type type)
{
    uint8_t *websocket_package;
    uint32_t ret_len, ret;

    /*websocket数据打包*/
    websocket_package = (uint8_t *)calloc(1, sizeof(char)*(data_len + 128));
    memset(websocket_package, 0, (data_len + 128));
    ret_len = w_enpackage(data, data_len, websocket_package, (data_len + 128), mod, type);
    ret = send(fd, websocket_package, ret_len, MSG_NOSIGNAL);
    free(websocket_package);
    return ret;
}

/**
 * websocket数据接收和基本解包
 *
 * @param int       fd              连接句柄
 * @param uint8_t   *data           数据接收地址
 * @param uint32_t  data_max_len    接收区可用最大长度
 *
 * @return int      <= 0 没有收到有效数据   > 0 成功接收并解包数据
 *
 */
static int w_recv(int fd, uint8_t *data, uint32_t data_max_len)
{
    uint8_t *websocket_package, *recv_buf;
    int ret, ret2 = 0;
    uint32_t ret_len = 0;

    recv_buf = (uint8_t *)calloc(1, sizeof(char)*data_max_len);
    memset(recv_buf, 0, data_max_len);
    ret = recv(fd, recv_buf, data_max_len, MSG_NOSIGNAL);
    if(ret > 0) {
        if(strncmp(recv_buf, "GET", 3) == 0) {
            ret2 = w_server_to_client(fd, recv_buf, ret);
            free(recv_buf);
            if(ret2 < 0) {
                memset(data, 0, data_max_len);
                printf("connect false !\r\n");
                return -1;
            }
            memset(data, 0, data_max_len);
            printf("retry connect ...\r\n");
            return -1;
        }

        /*websocket数据打包*/
        websocket_package = (uint8_t *)calloc(1, sizeof(char)*(ret + 128));
        memset(websocket_package, 0, (ret + 128));
        ret2 = w_depackage(recv_buf, ret, websocket_package, (ret + 128), &ret_len);
        /*解析为ping包, 自动回pong*/
        if(ret2 == WCT_PING && ret_len > 0) {
            w_send(fd, websocket_package, ret_len, true, WCT_PONG);
            /*显示数据*/
            printf("webSocket_recv : PING %d\r\n%s\r\n" , ret_len, websocket_package); 
            free(recv_buf);
            free(websocket_package);
            return WCT_NULL;
        } else if(ret_len > 0 && (ret2 == WCT_TXTDATA || ret2 == WCT_BINDATA || ret2 == WCT_MINDATA)) {
            /*解析为数据包*/
            /*把解析得到的数据复制出去*/
            memcpy(data, websocket_package, ret_len);
            strncpy(data, recv_buf, strlen(data));
            free(recv_buf);
            free(websocket_package);
            return ret_len;
        }
        free(recv_buf);
        free(websocket_package);
        return -ret;
    } else {
        free(recv_buf);
        return ret;
    }
}

int connect_socket(thread *threads, socket_info *socketinfo)
{
    threads->complete ++;

    int fd, ret, timeout;
    char buf[RECVBUF] = {"\0"}, send_text[REQUBUF] = "wbench testing", *p;

    fd = w_client_to_server(threads);
    if (fd < 0) {
        return fd;
    }

    socketinfo->fd = fd;

    if (threads->params[0] != '\0') {
        strcpy(send_text, threads->params); 
    }

    /*send(fd, send_text, strlen(send_text) + 1, 0);*/
    /*recv(fd, buf, RECVBUF, 0);*/
    ret = w_send(fd, send_text, strlen(send_text), true, WCT_TXTDATA);
    timeout = 0;
    while (true) {
        ret = w_recv(fd, buf, sizeof(buf));
        if (ret < 0) {
            if (++timeout > threads->timeout) {
                threads->errors.connect ++;
                /*printf("recv message timeout \n");*/
                break;
            }
        } else {
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


void get_host_by_name(char **host)
{
    struct hostent *hostinfo = gethostbyname(*host);
    int i = 0;
    for (i; hostinfo->h_addr_list[i]; i++) {
        /*printf("IP addr %d: %s \n", i + 1, inet_ntoa(*(struct in_addr*)hostinfo->h_addr_list[i]));*/
        strcpy(*host, inet_ntoa(*(struct in_addr*)hostinfo->h_addr_list[i]));
    }
}
