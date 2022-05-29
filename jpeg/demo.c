#include "tjpgd.h"
#include <stdio.h>
#include <stdint.h>

typedef struct {
    unsigned char *jpeg_buffer;
    int           jpeg_len;
    unsigned char *rgb565;
} tjpgd_encode_args_t;

static int __jpeg_data_in(JDEC *jd, uint8_t* buff, uint32_t ndata)
{
    tjpgd_encode_args_t *args = (tjpgd_encode_args_t *)jd->device;
    int ret;

    if (args->jpeg_len > ndata) {
        if (NULL != buff && ndata > 0) {
            memcpy(buff, args->jpeg_buffer, ndata);
        }

        args->jpeg_buffer += ndata;
        args->jpeg_len -= ndata;
        ret = ndata;
    } else {
        if (NULL != buff && args->jpeg_len > 0) {
            memcpy(buff, args->jpeg_buffer, args->jpeg_len);
        }

        ret = args->jpeg_len;
        args->jpeg_buffer += args->jpeg_len;
        args->jpeg_len = 0;
    }

    return ret;
}

static uint32_t JPEG_DECODE_WIDTH;
static uint32_t JPEG_DECODE_HEIGHT;
#define RGB565_PER_PIX_SIZE  (2)
static int __jpeg_data_out(JDEC *jd, void* bitmap, JRECT *rect)
{
    tjpgd_encode_args_t *args = (tjpgd_encode_args_t *)jd->device;
    uint8_t *src, *dst;
    uint16_t y, bws, bwd;

    src = (uint8_t *)bitmap;
    dst = args->rgb565 + RGB565_PER_PIX_SIZE * (rect->top * JPEG_DECODE_WIDTH + rect->left);
    bws = RGB565_PER_PIX_SIZE * (rect->right - rect->left + 1);
    bwd = RGB565_PER_PIX_SIZE * JPEG_DECODE_WIDTH;

    for (y = rect->top; y <= rect->bottom; y++) {
        memcpy(dst, src, bws);
        src += bws;
        dst += bwd;
    }

    return 1;
}

static int __jpeg_decode(unsigned char *jpeg_buffer, int jpeg_len, unsigned char *rgb565)
{
    JRESULT res;
    JDEC jdec = {0};
    uint8_t pool_buffer[3100];
    tjpgd_encode_args_t args;

    args.jpeg_buffer = jpeg_buffer;
    args.jpeg_len    = jpeg_len;
    args.rgb565      = rgb565;

    res = jd_prepare(&jdec, __jpeg_data_in, pool_buffer, sizeof(pool_buffer), &args);
    if (res != JDR_OK) {
        return -1;
    }

    JPEG_DECODE_HEIGHT = jdec.height;
    JPEG_DECODE_WIDTH  = jdec.width;
    assert(JPEG_DECODE_HEIGHT <= 240);
    assert(JPEG_DECODE_WIDTH <= 240);

    res = jd_decomp(&jdec, __jpeg_data_out, 0);
    if (res != JDR_OK) {
        return -1;
    }

    return 0;
}
