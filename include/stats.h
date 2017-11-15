#ifndef STATS_H
#define STATS_H

#ifndef HAVE_STDIO_H
#define HAVE_STDIO_H        1
#include <stdio.h>
#endif

#ifndef HAVE_STDLIB_H
#define HAVE_STDLIB_H       1
#include <stdlib.h>
#endif

#ifndef HAVE_STRING_H
#define HAVE_STRING_H       1
#include <string.h>
#endif

#ifndef HAVE_SYS_TIME_H
#define HAVE_SYS_TIME_H     1
#include <sys/time.h>
#endif

#include "config.h"

typedef struct {
    uint32_t connect;
    uint32_t read;
    uint32_t write;
    uint32_t status;
    uint32_t timeout;
} errors;

typedef struct SHA1Context{  
    unsigned Message_Digest[5];        
    unsigned Length_Low;               
    unsigned Length_High;              
    unsigned char Message_Block[64];   
    int Message_Block_Index;           
    int Computed;                      
    int Corrupted;                     
} SHA1Context;  

#define SHA1CircularShift(bits,word) ((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32-(bits))))  


uint64_t time_us();

int compare_fun(const void *param1, const void *param2);

void delayms(uint32_t ms);

void getRandomString(uint8_t *buf, uint32_t len);

int base64_encode(const uint8_t *bindata, char *base64, int binlength);

int base64_decode( const char *base64, uint8_t *bindata);

char * sha1_hash(const char *source);

int htoi(const char s[], int start, int len);

#endif
