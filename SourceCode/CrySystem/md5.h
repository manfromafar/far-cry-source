#ifndef MD5_H
#define MD5_H

#ifndef _CRY_COMMON_WIN32_SPECIFIC_HDR_
typedef unsigned int uint32;
#endif

struct MD5Context {
        uint32 buf[4];
        uint32 bits[2];
        unsigned char in[64];
};

#ifdef __cplusplus
extern "C"
{
#endif
extern void MD5Init( struct MD5Context *ctx );
extern void MD5Update( struct MD5Context *ctx,unsigned char *buf,unsigned len );
extern void MD5Final( unsigned char digest[16],struct MD5Context *ctx );
extern void MD5Transform( uint32 buf[4],uint32 in[16] );

#ifdef __cplusplus
};
#endif

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;

#endif /* !MD5_H */
