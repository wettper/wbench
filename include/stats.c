#include "stats.h"

uint64_t time_us()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (t.tv_sec * 1000000) + t.tv_usec;
}

int compare_fun(const void *param1, const void *param2)
{
    return *(int*)param1 - *(int*)param2;
}

void delayms(uint32_t ms)
{
    struct timeval t;
    t.tv_sec    = ms / 1000;
    t.tv_usec   = (ms%1000)*1000;
    select(0, NULL, NULL, NULL, &t);
}

void getRandomString(uint8_t *buf, uint32_t len)
{
    uint32_t i;
    uint8_t temp;
    srand((int)time(0));
    for (i = 0; i < len; i ++) {
        temp = (uint8_t)(rand()%256);
        if (temp == 0) {
            temp = 128;
        }
        buf[i] = temp;
    }
}

//base64编/解码用的基础字符集
const char base64char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*******************************************************************************
 * 名称: base64_encode
 * 功能: ascii编码为base64格式
 * 形参: bindata : ascii字符串输入
 *            base64 : base64字符串输出
 *          binlength : bindata的长度
 * 返回: base64字符串长度
 * 说明: 无
 ******************************************************************************/
int base64_encode( const uint8_t *bindata, char *base64, int binlength)
{
    int i, j;
    uint8_t current;
    for ( i = 0, j = 0 ; i < binlength ; i += 3 )
    {
        current = (bindata[i] >> 2) ;
        current &= (uint8_t)0x3F;
        base64[j++] = base64char[(int)current];
        current = ( (uint8_t)(bindata[i] << 4 ) ) & ( (uint8_t)0x30 ) ;
        if ( i + 1 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ( (uint8_t)(bindata[i+1] >> 4) ) & ( (uint8_t) 0x0F );
        base64[j++] = base64char[(int)current];
        current = ( (uint8_t)(bindata[i+1] << 2) ) & ( (uint8_t)0x3C ) ;
        if ( i + 2 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ( (uint8_t)(bindata[i+2] >> 6) ) & ( (uint8_t) 0x03 );
        base64[j++] = base64char[(int)current];
        current = ( (uint8_t)bindata[i+2] ) & ( (uint8_t)0x3F ) ;
        base64[j++] = base64char[(int)current];
    }
    base64[j] = '\0';
    return j;
}
/*******************************************************************************
 * 名称: base64_decode
 * 功能: base64格式解码为ascii
 * 形参: base64 : base64字符串输入
 *            bindata : ascii字符串输出
 * 返回: 解码出来的ascii字符串长度
 * 说明: 无
 ******************************************************************************/
int base64_decode( const char *base64, uint8_t *bindata)
{
    int i, j;
    uint8_t k;
    uint8_t temp[4];
    for ( i = 0, j = 0; base64[i] != '\0' ; i += 4 )
    {
        memset( temp, 0xFF, sizeof(temp) );
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i] )
                temp[0]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+1] )
                temp[1]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+2] )
                temp[2]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+3] )
                temp[3]= k;
        }
        bindata[j++] = ((uint8_t)(((uint8_t)(temp[0] << 2))&0xFC)) | \
                       ((uint8_t)((uint8_t)(temp[1]>>4)&0x03));
        if ( base64[i+2] == '=' )
            break;
        bindata[j++] = ((uint8_t)(((uint8_t)(temp[1] << 4))&0xF0)) | \
                       ((uint8_t)((uint8_t)(temp[2]>>2)&0x0F));
        if ( base64[i+3] == '=' )
            break;
        bindata[j++] = ((uint8_t)(((uint8_t)(temp[2] << 6))&0xF0)) | \
                       ((uint8_t)(temp[3]&0x3F));
    }
    return j;
}

static void SHA1ProcessMessageBlock(SHA1Context *context)
{
    const unsigned K[] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
    int         t;
    unsigned    temp;
    unsigned    W[80];
    unsigned    A, B, C, D, E;

    for(t = 0; t < 16; t++) {
        W[t] = ((unsigned) context->Message_Block[t * 4]) << 24;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 3]);
    }

    for(t = 16; t < 80; t++)
        W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);

    A = context->Message_Digest[0];
    B = context->Message_Digest[1];
    C = context->Message_Digest[2];
    D = context->Message_Digest[3];
    E = context->Message_Digest[4];
    
    for(t = 0; t < 20; t++) {
        temp =  SHA1CircularShift(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;  
    }
    
    for(t = 20; t < 40; t++) {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }
    
    for(t = 40; t < 60; t++) {
        temp = SHA1CircularShift(5,A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }
    
    for(t = 60; t < 80; t++) {  
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }
    context->Message_Digest[0] = (context->Message_Digest[0] + A) & 0xFFFFFFFF;
    context->Message_Digest[1] = (context->Message_Digest[1] + B) & 0xFFFFFFFF;
    context->Message_Digest[2] = (context->Message_Digest[2] + C) & 0xFFFFFFFF;
    context->Message_Digest[3] = (context->Message_Digest[3] + D) & 0xFFFFFFFF;
    context->Message_Digest[4] = (context->Message_Digest[4] + E) & 0xFFFFFFFF;
    context->Message_Block_Index = 0;
}

static void SHA1Reset(SHA1Context *context)
{
    context->Length_Low             = 0;
    context->Length_High            = 0;
    context->Message_Block_Index    = 0;

    context->Message_Digest[0]      = 0x67452301;
    context->Message_Digest[1]      = 0xEFCDAB89;
    context->Message_Digest[2]      = 0x98BADCFE;
    context->Message_Digest[3]      = 0x10325476;
    context->Message_Digest[4]      = 0xC3D2E1F0;

    context->Computed   = 0;
    context->Corrupted  = 0;
}  

static void SHA1PadMessage(SHA1Context *context)
{
    if (context->Message_Block_Index > 55) {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 64)  context->Message_Block[context->Message_Block_Index++] = 0;
        SHA1ProcessMessageBlock(context);
        while(context->Message_Block_Index < 56) context->Message_Block[context->Message_Block_Index++] = 0;
    } else {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 56) context->Message_Block[context->Message_Block_Index++] = 0;
    }
    context->Message_Block[56] = (context->Length_High >> 24 ) & 0xFF;
    context->Message_Block[57] = (context->Length_High >> 16 ) & 0xFF;
    context->Message_Block[58] = (context->Length_High >> 8 ) & 0xFF;
    context->Message_Block[59] = (context->Length_High) & 0xFF;
    context->Message_Block[60] = (context->Length_Low >> 24 ) & 0xFF;
    context->Message_Block[61] = (context->Length_Low >> 16 ) & 0xFF;
    context->Message_Block[62] = (context->Length_Low >> 8 ) & 0xFF;
    context->Message_Block[63] = (context->Length_Low) & 0xFF;

    SHA1ProcessMessageBlock(context);
} 

static int SHA1Result(SHA1Context *context)
{
    if (context->Corrupted) {
        return 0;
    }
    
    if (!context->Computed) {
        SHA1PadMessage(context);
        context->Computed = 1;
    }
    return 1;
}

static void SHA1Input(SHA1Context *context,const char *message_array,unsigned length)
{
    if (!length)
        return;
    
    if (context->Computed || context->Corrupted) {
        context->Corrupted = 1;
        return;
    }
    
    while(length-- && !context->Corrupted) {
        context->Message_Block[context->Message_Block_Index++] = (*message_array & 0xFF);

        context->Length_Low += 8;
        context->Length_Low &= 0xFFFFFFFF;

        if (context->Length_Low == 0) {
            context->Length_High++;
            context->Length_High &= 0xFFFFFFFF;
            if (context->Length_High == 0) context->Corrupted = 1;
        }
        
        if (context->Message_Block_Index == 64) {
            SHA1ProcessMessageBlock(context);
        }
        message_array++;
    }
}

char * sha1_hash(const char *source)
{
    SHA1Context sha;
    char *buf;
    
    SHA1Reset(&sha);
    SHA1Input(&sha, source, strlen(source));
    
    if (!SHA1Result(&sha)) {
        printf("SHA1 ERROR: Could not compute message digest");
        return NULL;
    } else {
        buf = (char *)malloc(128);
        memset(buf, 0, 128);
        sprintf(buf, "%08X%08X%08X%08X%08X", sha.Message_Digest[0],sha.Message_Digest[1],
                sha.Message_Digest[2],sha.Message_Digest[3],sha.Message_Digest[4]);
        return buf;
    }
}

static int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + 'a' - 'A';
    } else {
        return c;
    }
}

int htoi(const char s[], int start, int len)
{
    int i, j;
    int n = 0;

    /*判断是否有前导0x或者0X*/
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X')) {
        i = 2;
    } else {
        i = 0;
    }
    
    i += start;
    j = 0;
    for (; (s[i] >= '0' && s[i] <= '9') 
            || (s[i] >= 'a' && s[i] <= 'f') || (s[i] >='A' && s[i] <= 'F');++i) {
        if(j>=len) {
            break;
        }
        
        if (tolower(s[i]) > '9') {
            n = 16 * n + (10 + tolower(s[i]) - 'a');
        } else {
            n = 16 * n + (tolower(s[i]) - '0');
        }
        j++;
    }
    return n;
}
