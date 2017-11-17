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

void get_random_string(uint8_t *buf, uint32_t len)
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

/*base64编/解码用的基础字符集*/
const char base64char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * ascii编码为base64格式
 *
 * @param const uint8_t bindata     ascii字符串输入
 * @param char          base64      base64字符串输出
 * @param int           binlength   bindata的长度
 *
 * @return int          base64字符串长度
 *
 */
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

/**
 * base64格式解码为ascii
 *
 * @param const char    *base64     base64字符串输入
 * @param uint8_t       *bindata    ascii字符串输出
 * 
 * @return int          解码出来的ascii字符串长度
 *
 */
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

static void sha1_process_message_block(sha1_context *context)
{
    const unsigned k[] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
    int         t;
    unsigned    temp;
    unsigned    w[80];
    unsigned    a, b, c, d, e;

    for(t = 0; t < 16; t++) {
        w[t] = ((unsigned) context->message_block[t * 4]) << 24;
        w[t] |= ((unsigned) context->message_block[t * 4 + 1]) << 16;
        w[t] |= ((unsigned) context->message_block[t * 4 + 2]) << 8;
        w[t] |= ((unsigned) context->message_block[t * 4 + 3]);
    }

    for(t = 16; t < 80; t++)
        w[t] = sha1_circular_shift(1, w[t-3] ^ w[t-8] ^ w[t-14] ^ w[t-16]);

    a = context->message_digest[0];
    b = context->message_digest[1];
    c = context->message_digest[2];
    d = context->message_digest[3];
    e = context->message_digest[4];
    
    for(t = 0; t < 20; t++) {
        temp =  sha1_circular_shift(5, a) + ((b & c) | ((~b) & d)) + e + w[t] + k[0];
        temp &= 0xFFFFFFFF;
        e = d;
        d = c;
        c = sha1_circular_shift(30, b);
        b = a;
        a = temp;  
    }
    
    for(t = 20; t < 40; t++) {
        temp = sha1_circular_shift(5, a) + (b ^ c ^ d) + e + w[t] + k[1];
        temp &= 0xFFFFFFFF;
        e = d;
        d = c;
        c = sha1_circular_shift(30, b);
        b = a;
        a = temp;
    }
    
    for(t = 40; t < 60; t++) {
        temp = sha1_circular_shift(5, a) + ((b & c) | (b & d) | (c & d)) + e + w[t] + k[2];
        temp &= 0xFFFFFFFF;
        e = d;
        d = c;
        c = sha1_circular_shift(30, b);
        b = a;
        a = temp;
    }
    
    for(t = 60; t < 80; t++) {  
        temp = sha1_circular_shift(5, a) + (b ^ c ^ d) + e + w[t] + k[3];
        temp &= 0xFFFFFFFF;
        e = d;
        d = c;
        c = sha1_circular_shift(30, b);
        b = a;
        a = temp;
    }
    context->message_digest[0] = (context->message_digest[0] + a) & 0xFFFFFFFF;
    context->message_digest[1] = (context->message_digest[1] + b) & 0xFFFFFFFF;
    context->message_digest[2] = (context->message_digest[2] + c) & 0xFFFFFFFF;
    context->message_digest[3] = (context->message_digest[3] + d) & 0xFFFFFFFF;
    context->message_digest[4] = (context->message_digest[4] + e) & 0xFFFFFFFF;
    context->message_block_index = 0;
}

static void sha1_reset(sha1_context *context)
{
    context->length_low             = 0;
    context->length_high            = 0;
    context->message_block_index    = 0;

    context->message_digest[0]      = 0x67452301;
    context->message_digest[1]      = 0xEFCDAB89;
    context->message_digest[2]      = 0x98BADCFE;
    context->message_digest[3]      = 0x10325476;
    context->message_digest[4]      = 0xC3D2E1F0;

    context->computed   = 0;
    context->corrupted  = 0;
}  

static void sha1_pad_message(sha1_context *context)
{
    if (context->message_block_index > 55) {
        context->message_block[context->message_block_index++] = 0x80;
        while(context->message_block_index < 64)  context->message_block[context->message_block_index++] = 0;
        sha1_process_message_block(context);
        while(context->message_block_index < 56) context->message_block[context->message_block_index++] = 0;
    } else {
        context->message_block[context->message_block_index++] = 0x80;
        while(context->message_block_index < 56) context->message_block[context->message_block_index++] = 0;
    }
    context->message_block[56] = (context->length_high >> 24 ) & 0xFF;
    context->message_block[57] = (context->length_high >> 16 ) & 0xFF;
    context->message_block[58] = (context->length_high >> 8 ) & 0xFF;
    context->message_block[59] = (context->length_high) & 0xFF;
    context->message_block[60] = (context->length_low >> 24 ) & 0xFF;
    context->message_block[61] = (context->length_low >> 16 ) & 0xFF;
    context->message_block[62] = (context->length_low >> 8 ) & 0xFF;
    context->message_block[63] = (context->length_low) & 0xFF;

    sha1_process_message_block(context);
} 

static int sha1_result(sha1_context *context)
{
    if (context->corrupted) {
        return 0;
    }
    
    if (!context->computed) {
        sha1_pad_message(context);
        context->computed = 1;
    }
    return 1;
}

static void sha1_input(sha1_context *context, const char *message_array, unsigned length)
{
    if (!length)
        return;
    
    if (context->computed || context->corrupted) {
        context->corrupted = 1;
        return;
    }
    
    while(length-- && !context->corrupted) {
        context->message_block[context->message_block_index++] = (*message_array & 0xFF);

        context->length_low += 8;
        context->length_low &= 0xFFFFFFFF;

        if (context->length_low == 0) {
            context->length_high++;
            context->length_high &= 0xFFFFFFFF;
            if (context->length_high == 0) context->corrupted = 1;
        }
        
        if (context->message_block_index == 64) {
            sha1_process_message_block(context);
        }
        message_array++;
    }
}

char * sha1_hash(const char *source)
{
    sha1_context sha;
    char *buf;
    
    sha1_reset(&sha);
    sha1_input(&sha, source, strlen(source));
    
    if (!sha1_result(&sha)) {
        printf("SHA1 ERROR: Could not compute message digest");
        return NULL;
    } else {
        buf = (char *)malloc(128);
        memset(buf, 0, 128);
        sprintf(buf, "%08X%08X%08X%08X%08X", sha.message_digest[0], sha.message_digest[1],
                sha.message_digest[2],sha.message_digest[3], sha.message_digest[4]);
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
