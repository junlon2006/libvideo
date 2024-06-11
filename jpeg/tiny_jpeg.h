#ifndef TJE_IMPLEMENTATION
#define TJE_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define JPE_TAG "[JPEG]"

#define TJEI_BUFFER_SIZE 1024
#define tjei_min(a, b) ((a) < b) ? (a) : (b);
#define tjei_max(a, b) ((a) < b) ? (b) : (a);
#define CHAR_BIT 8
#define ABS(x) ((x) < 0 ? -(x) : (x))

enum {
    TJEI_LUMA_DC,
    TJEI_LUMA_AC,
    TJEI_CHROMA_DC,
    TJEI_CHROMA_AC,
};

struct TJEProcessedQT
{
    float chroma[64];
    float luma[64];
};

typedef enum
{
    TJE_RGBA = 4,
    TJE_RGB888 = 3,
    TJE_RGB565 = 2,
} TJEColorFormat;

typedef void tje_write_func(void* context, void* data, int size);

typedef struct
{
    void*           context;
    tje_write_func* func;
} TJEWriteContext;

typedef struct
{
    uint8_t         ehuffsize[4][257];
    uint16_t        ehuffcode[4][256];
    uint8_t const * ht_bits[4];
    uint8_t const * ht_vals[4];
    uint8_t         qt_luma[64];
    uint8_t         qt_chroma[64];
    TJEWriteContext write_context;
    size_t          output_buffer_count;
    uint8_t         output_buffer[TJEI_BUFFER_SIZE];
} TJEState;

static const uint8_t tjei_default_qt_luma_from_spec[] =
{
   16,11,10,16, 24, 40, 51, 61,
   12,12,14,19, 26, 58, 60, 55,
   14,13,16,24, 40, 57, 69, 56,
   14,17,22,29, 51, 87, 80, 62,
   18,22,37,56, 68,109,103, 77,
   24,35,55,64, 81,104,113, 92,
   49,64,78,87,103,121,120,101,
   72,92,95,98,112,100,103, 99,
};

static const uint8_t tjei_default_qt_chroma_from_paper[] =
{
    16,  12, 14,  14, 18, 24,  49,  72,
    11,  10, 16,  24, 40, 51,  61,  12,
    13,  17, 22,  35, 64, 92,  14,  16,
    22,  37, 55,  78, 95, 19,  24,  29,
    56,  64, 87,  98, 26, 40,  51,  68,
    81, 103, 112, 58, 57, 87,  109, 104,
    121,100, 60,  69, 80, 103, 113, 120,
    103, 55, 56,  62, 77, 92,  101, 99,
};

static const uint8_t tjei_default_ht_luma_dc_len[16] = { 0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
static const uint8_t tjei_default_ht_luma_dc[12] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
static const uint8_t tjei_default_ht_chroma_dc_len[16] = { 0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };
static const uint8_t tjei_default_ht_chroma_dc[12] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
static const uint8_t tjei_default_ht_luma_ac_len[16] = { 0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };

static const uint8_t tjei_default_ht_luma_ac[] =
{
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
    0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
    0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA
};

static const uint8_t tjei_default_ht_chroma_ac_len[16] = { 0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77 };

static const uint8_t tjei_default_ht_chroma_ac[] =
{
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
    0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
    0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
    0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
    0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA
};

static const uint8_t tjei_zig_zag[64] =
{
    0,   1,  5,  6, 14, 15, 27, 28,
    2,   4,  7, 13, 16, 26, 29, 42,
    3,   8, 12, 17, 25, 30, 41, 43,
    9,  11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63,
};

static uint16_t tjei_be_word(const uint16_t native_word)
{
    uint8_t bytes[2];
    uint16_t result;
    bytes[1] = (native_word & 0x00ff);
    bytes[0] = ((native_word & 0xff00) >> 8);
    memcpy(&result, bytes, sizeof(bytes));
    return result;
}

static const uint8_t tjeik_jfif_id[] = "JFIF";
static const uint8_t tjeik_com_str[] = "Created by Tiny JPEG Encoder";

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    uint16_t SOI;
    // JFIF header.
    uint16_t APP0;
    uint16_t jfif_len;
    uint8_t  jfif_id[5];
    uint16_t version;
    uint8_t  units;
    uint16_t x_density;
    uint16_t y_density;
    uint8_t  x_thumb;
    uint8_t  y_thumb;
} TJEJPEGHeader;

typedef struct
{
    uint16_t com;
    uint16_t com_len;
    char     com_str[sizeof(tjeik_com_str) - 1];
} TJEJPEGComment;

typedef struct
{
    uint8_t component_id;
    uint8_t sampling_factors;
    uint8_t qt;
} TJEComponentSpec;

typedef struct
{
    uint16_t         SOF;
    uint16_t         len;
    uint8_t          precision;
    uint16_t         height;
    uint16_t         width;
    uint8_t          num_components;
    TJEComponentSpec component_spec[3];
} TJEFrameHeader;

typedef struct
{
    uint8_t component_id;
    uint8_t dc_ac;
} TJEFrameComponentSpec;

typedef struct
{
    uint16_t              SOS;
    uint16_t              len;
    uint8_t               num_components;
    TJEFrameComponentSpec component_spec[3];
    uint8_t               first;
    uint8_t               last;
    uint8_t               ah_al;
} TJEScanHeader;
#pragma pack(pop)

static void tjei_write(TJEState* state, const void* data, size_t num_bytes, size_t num_elements)
{
    size_t to_write = num_bytes * num_elements;
    size_t capped_count = tjei_min(to_write, TJEI_BUFFER_SIZE - 1 - state->output_buffer_count);

    memcpy(state->output_buffer + state->output_buffer_count, data, capped_count);
    state->output_buffer_count += capped_count;

    assert (state->output_buffer_count <= TJEI_BUFFER_SIZE - 1);

    if ( state->output_buffer_count == TJEI_BUFFER_SIZE - 1 ) {
        state->write_context.func(state->write_context.context, state->output_buffer, (int)state->output_buffer_count);
        state->output_buffer_count = 0;
    }

    if (capped_count < to_write) {
        tjei_write(state, (uint8_t*)data+capped_count, to_write - capped_count, 1);
    }
}

static void tjei_write_DQT(TJEState* state, const uint8_t* matrix, uint8_t id)
{
    uint16_t DQT = tjei_be_word(0xffdb);
    uint16_t len;
    uint8_t precision_and_id;
    tjei_write(state, &DQT, sizeof(uint16_t), 1);
    len = tjei_be_word(0x0043); // 2(len) + 1(id) + 64(matrix) = 67 = 0x43
    tjei_write(state, &len, sizeof(uint16_t), 1);
    assert(id < 4);
    precision_and_id = id;  // 0x0000 8 bits | 0x00id
    tjei_write(state, &precision_and_id, sizeof(uint8_t), 1);
    tjei_write(state, matrix, 64*sizeof(uint8_t), 1);
}

typedef enum
{
    TJEI_DC = 0,
    TJEI_AC = 1
} TJEHuffmanTableClass;

static void tjei_write_DHT(TJEState* state,
                           uint8_t const * matrix_len,
                           uint8_t const * matrix_val,
                           TJEHuffmanTableClass ht_class,
                           uint8_t id)
{
    int num_values = 0;
    uint16_t DHT;
    uint16_t len;
    uint8_t tc_th;
    int i;

    for ( i = 0; i < 16; ++i ) {
        num_values += matrix_len[i];
    }
    assert(num_values <= 0xffff);

    DHT = tjei_be_word(0xffc4);
    len = tjei_be_word(2 + 1 + 16 + (uint16_t)num_values);
    assert(id < 4);
    tc_th = (uint8_t)((((uint8_t)ht_class) << 4) | id);

    tjei_write(state, &DHT, sizeof(uint16_t), 1);
    tjei_write(state, &len, sizeof(uint16_t), 1);
    tjei_write(state, &tc_th, sizeof(uint8_t), 1);
    tjei_write(state, matrix_len, sizeof(uint8_t), 16);
    tjei_write(state, matrix_val, sizeof(uint8_t), (size_t)num_values);
}

static uint8_t* tjei_huff_get_code_lengths(uint8_t huffsize[/*256*/], uint8_t const * bits)
{
    int k = 0, i, j;
    for ( i = 0; i < 16; ++i ) {
        for ( j = 0; j < bits[i]; ++j ) {
            huffsize[k++] = (uint8_t)(i + 1);
        }
        huffsize[k] = 0;
    }
    return huffsize;
}

static uint16_t* tjei_huff_get_codes(uint16_t codes[], uint8_t* huffsize, int64_t count)
{
    uint16_t code = 0;
    int k = 0;
    uint8_t sz = huffsize[0];
    for(;;) {
        do {
            assert(k < count);
            codes[k++] = code++;
        } while (huffsize[k] == sz);
        if (huffsize[k] == 0) {
            return codes;
        }
        do {
            code = (uint16_t)(code << 1);
            ++sz;
        } while( huffsize[k] != sz );
    }
}

static void tjei_huff_get_extended(uint8_t* out_ehuffsize,
                                   uint16_t* out_ehuffcode,
                                   uint8_t const * huffval,
                                   uint8_t* huffsize,
                                   uint16_t* huffcode, int64_t count)
{
    int k = 0;
    uint8_t val;
    do {
        val = huffval[k];
        out_ehuffcode[val] = huffcode[k];
        out_ehuffsize[val] = huffsize[k];
        k++;
    } while ( k < count );
}

static void tjei_calculate_variable_length_int(int value, uint16_t out[2])
{
    int abs_val = value;
    if ( value < 0 ) {
        abs_val = -abs_val;
        --value;
    }
    out[1] = 1;
    while( abs_val >>= 1 ) {
        ++out[1];
    }
    out[0] = (uint16_t)(value & ((1 << out[1]) - 1));
}

static void tjei_write_bits(TJEState* state,
                            uint32_t* bitbuffer, uint32_t* location,
                            uint16_t num_bits, uint16_t bits)
{
    uint32_t nloc = *location + num_bits;
    uint8_t c;
    *bitbuffer |= (uint32_t)(bits << (32 - nloc));
    *location = nloc;
    while ( *location >= 8 ) {
        c = (uint8_t)((*bitbuffer) >> 24);
        tjei_write(state, &c, 1, 1);
        if ( c == 0xff )  {
            char z = 0;
            tjei_write(state, &z, 1, 1);
        }
        *bitbuffer <<= 8;
        *location -= 8;
    }
}

static void tjei_fdct (float * data)
{
    float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    float tmp10, tmp11, tmp12, tmp13;
    float z1, z2, z3, z4, z5, z11, z13;
    float *dataptr;
    int ctr;

    dataptr = data;
    for ( ctr = 7; ctr >= 0; ctr-- ) {
        tmp0 = dataptr[0] + dataptr[7];
        tmp7 = dataptr[0] - dataptr[7];
        tmp1 = dataptr[1] + dataptr[6];
        tmp6 = dataptr[1] - dataptr[6];
        tmp2 = dataptr[2] + dataptr[5];
        tmp5 = dataptr[2] - dataptr[5];
        tmp3 = dataptr[3] + dataptr[4];
        tmp4 = dataptr[3] - dataptr[4];

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        dataptr[0] = tmp10 + tmp11;
        dataptr[4] = tmp10 - tmp11;

        z1 = (tmp12 + tmp13) * ((float) 0.707106781);
        dataptr[2] = tmp13 + z1;
        dataptr[6] = tmp13 - z1;

        tmp10 = tmp4 + tmp5;
        tmp11 = tmp5 + tmp6;
        tmp12 = tmp6 + tmp7;

        z5 = (tmp10 - tmp12) * ((float) 0.382683433);
        z2 = ((float) 0.541196100) * tmp10 + z5;
        z4 = ((float) 1.306562965) * tmp12 + z5;
        z3 = tmp11 * ((float) 0.707106781);

        z11 = tmp7 + z3;
        z13 = tmp7 - z3;

        dataptr[5] = z13 + z2;
        dataptr[3] = z13 - z2;
        dataptr[1] = z11 + z4;
        dataptr[7] = z11 - z4;

        dataptr += 8;
    }

    dataptr = data;
    for ( ctr = 8-1; ctr >= 0; ctr-- ) {
        tmp0 = dataptr[8*0] + dataptr[8*7];
        tmp7 = dataptr[8*0] - dataptr[8*7];
        tmp1 = dataptr[8*1] + dataptr[8*6];
        tmp6 = dataptr[8*1] - dataptr[8*6];
        tmp2 = dataptr[8*2] + dataptr[8*5];
        tmp5 = dataptr[8*2] - dataptr[8*5];
        tmp3 = dataptr[8*3] + dataptr[8*4];
        tmp4 = dataptr[8*3] - dataptr[8*4];

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        dataptr[8*0] = tmp10 + tmp11;
        dataptr[8*4] = tmp10 - tmp11;

        z1 = (tmp12 + tmp13) * ((float) 0.707106781);
        dataptr[8*2] = tmp13 + z1;
        dataptr[8*6] = tmp13 - z1;

        tmp10 = tmp4 + tmp5;
        tmp11 = tmp5 + tmp6;
        tmp12 = tmp6 + tmp7;

        z5 = (tmp10 - tmp12) * ((float) 0.382683433);
        z2 = ((float) 0.541196100) * tmp10 + z5;
        z4 = ((float) 1.306562965) * tmp12 + z5;
        z3 = tmp11 * ((float) 0.707106781);

        z11 = tmp7 + z3;
        z13 = tmp7 - z3;

        dataptr[8*5] = z13 + z2;
        dataptr[8*3] = z13 - z2;
        dataptr[8*1] = z11 + z4;
        dataptr[8*7] = z11 - z4;

        dataptr++;
    }
}

static void tjei_encode_and_write_MCU(TJEState* state,
                                      float* mcu,
                                      float* qt,  // Pre-processed quantization matrix.
                                      uint8_t* huff_dc_len, uint16_t* huff_dc_code, // Huffman tables
                                      uint8_t* huff_ac_len, uint16_t* huff_ac_code,
                                      int* pred,  // Previous DC coefficient
                                      uint32_t* bitbuffer,  // Bitstack.
                                      uint32_t* location)
{
    int du[64];
    float dct_mcu[64];
    int i;
    float fval;
    int val;
    uint16_t vli[2];
    int diff;
    int last_non_zero_i = 0;
    int zero_count = 0;
    uint16_t sym1;

    memcpy(dct_mcu, mcu, 64 * sizeof(float));

    tjei_fdct(dct_mcu);
    for ( i = 0; i < 64; ++i ) {
        fval = dct_mcu[i];
        fval *= qt[i];
        fval = floorf(fval + 1024 + 0.5f);
        fval -= 1024;
        val = (int)fval;
        du[tjei_zig_zag[i]] = val;
    }

    diff = du[0] - *pred;
    *pred = du[0];
    if ( diff != 0 ) {
        tjei_calculate_variable_length_int(diff, vli);
        tjei_write_bits(state, bitbuffer, location, huff_dc_len[vli[1]], huff_dc_code[vli[1]]);
        tjei_write_bits(state, bitbuffer, location, vli[1], vli[0]);
    } else {
        tjei_write_bits(state, bitbuffer, location, huff_dc_len[0], huff_dc_code[0]);
    }

    for ( i = 63; i > 0; --i ) {
        if (du[i] != 0) {
            last_non_zero_i = i;
            break;
        }
    }

    for ( i = 1; i <= last_non_zero_i; ++i ) {
        zero_count = 0;
        while ( du[i] == 0 ) {
            ++zero_count;
            ++i;
            if (zero_count == 16) {
                tjei_write_bits(state, bitbuffer, location, huff_ac_len[0xf0], huff_ac_code[0xf0]);
                zero_count = 0;
            }
        }
        tjei_calculate_variable_length_int(du[i], vli);

        assert(zero_count < 0x10);
        assert(vli[1] <= 10);

        sym1 = (uint16_t)((uint16_t)zero_count << 4) | vli[1];

        assert(huff_ac_len[sym1] != 0);

        tjei_write_bits(state, bitbuffer, location, huff_ac_len[sym1], huff_ac_code[sym1]);
        tjei_write_bits(state, bitbuffer, location, vli[1], vli[0]);
    }

    if (last_non_zero_i != 63) {
        tjei_write_bits(state, bitbuffer, location, huff_ac_len[0], huff_ac_code[0]);
    }
    return;
}

static void tjei_huff_expand(TJEState* state)
{
    int32_t spec_tables_len[4] = { 0 };
    int i, k;
    uint8_t huffsize[4][257];
    uint16_t huffcode[4][256];
    int64_t count;

    assert(state);

    state->ht_bits[TJEI_LUMA_DC]   = tjei_default_ht_luma_dc_len;
    state->ht_bits[TJEI_LUMA_AC]   = tjei_default_ht_luma_ac_len;
    state->ht_bits[TJEI_CHROMA_DC] = tjei_default_ht_chroma_dc_len;
    state->ht_bits[TJEI_CHROMA_AC] = tjei_default_ht_chroma_ac_len;

    state->ht_vals[TJEI_LUMA_DC]   = tjei_default_ht_luma_dc;
    state->ht_vals[TJEI_LUMA_AC]   = tjei_default_ht_luma_ac;
    state->ht_vals[TJEI_CHROMA_DC] = tjei_default_ht_chroma_dc;
    state->ht_vals[TJEI_CHROMA_AC] = tjei_default_ht_chroma_ac;

    for ( i = 0; i < 4; ++i ) {
        for ( k = 0; k < 16; ++k ) {
            spec_tables_len[i] += state->ht_bits[i][k];
        }
    }

    for ( i = 0; i < 4; ++i ) {
        assert (256 >= spec_tables_len[i]);
        tjei_huff_get_code_lengths(huffsize[i], state->ht_bits[i]);
        tjei_huff_get_codes(huffcode[i], huffsize[i], spec_tables_len[i]);
    }

    for ( i = 0; i < 4; ++i ) {
        count = spec_tables_len[i];
        tjei_huff_get_extended(state->ehuffsize[i],
                               state->ehuffcode[i],
                               state->ht_vals[i],
                               &huffsize[i][0],
                               &huffcode[i][0], count);
    }
}

static int tjei_encode_main(TJEState* state,
                            const unsigned char* src_data,
                            const int width,
                            const int height,
                            TJEColorFormat color_format)
{
    struct TJEProcessedQT pqt;
    static const float aan_scales[] = { 1.0f, 1.387039845f, 1.306562965f, 1.175875602f, 1.0f, 0.785694958f, 0.541196100f, 0.275899379f };
    int x, y, i;
    TJEJPEGHeader header;
    uint16_t jfif_len;
    TJEJPEGComment com;
    uint16_t com_len;
    TJEFrameHeader frame_header;
    TJEComponentSpec spec;
    uint8_t tables[3] = { 0, 1, 1};
    TJEScanHeader scan_header;
    uint8_t scan_tables[3] = { 0x00, 0x11, 0x11 };
    TJEFrameComponentSpec cs;
    uint32_t bitbuffer = 0;
    uint32_t location = 0;
    float du_y[64];
    float du_b[64];
    float du_r[64];
    int pred_y = 0;
    int pred_b = 0;
    int pred_r = 0;
    int off_y, off_x;
    int block_index, src_index;
    int col, row;
    uint8_t r,g,b;
    uint16_t rgb;
    float luma, cb, cr;
    uint16_t EOI;

    if (color_format < 2 || color_format > 4) {
        return 0;
    }

    if (width > 0xffff || height > 0xffff) {
        return 0;
    }

    for(y=0; y<8; y++) {
        for(x=0; x<8; x++) {
            i = y*8 + x;
            pqt.luma[y*8+x] = 1.0f / (8 * aan_scales[x] * aan_scales[y] * state->qt_luma[tjei_zig_zag[i]]);
            pqt.chroma[y*8+x] = 1.0f / (8 * aan_scales[x] * aan_scales[y] * state->qt_chroma[tjei_zig_zag[i]]);
        }
    }

    {
        header.SOI = tjei_be_word(0xffd8);  // Sequential DCT
        header.APP0 = tjei_be_word(0xffe0);

        jfif_len = sizeof(TJEJPEGHeader) - 4 /*SOI & APP0 markers*/;
        header.jfif_len = tjei_be_word(jfif_len);
        memcpy(header.jfif_id, (void*)tjeik_jfif_id, 5);
        header.version = tjei_be_word(0x0102);
        header.units = 0x01;  // Dots-per-inch
        header.x_density = tjei_be_word(0x0060);  // 96 DPI
        header.y_density = tjei_be_word(0x0060);  // 96 DPI
        header.x_thumb = 0;
        header.y_thumb = 0;
        tjei_write(state, &header, sizeof(TJEJPEGHeader), 1);
    }

    {
        com_len = 2 + sizeof(tjeik_com_str) - 1;
        com.com = tjei_be_word(0xfffe);
        com.com_len = tjei_be_word(com_len);
        memcpy(com.com_str, (void*)tjeik_com_str, sizeof(tjeik_com_str)-1);
        tjei_write(state, &com, sizeof(TJEJPEGComment), 1);
    }

    tjei_write_DQT(state, state->qt_luma, 0x00);
    tjei_write_DQT(state, state->qt_chroma, 0x01);

    {
        frame_header.SOF = tjei_be_word(0xffc0);
        frame_header.len = tjei_be_word(8 + 3 * 3);
        frame_header.precision = 8;
        assert(width <= 0xffff);
        assert(height <= 0xffff);
        frame_header.width = tjei_be_word((uint16_t)width);
        frame_header.height = tjei_be_word((uint16_t)height);
        frame_header.num_components = 3;

        for (i = 0; i < 3; ++i) {
            spec.component_id = (uint8_t)(i + 1);  // No particular reason. Just 1, 2, 3.
            spec.sampling_factors = (uint8_t)0x11;
            spec.qt = tables[i];

            frame_header.component_spec[i] = spec;
        }
        tjei_write(state, &frame_header, sizeof(TJEFrameHeader), 1);
    }

    tjei_write_DHT(state, state->ht_bits[TJEI_LUMA_DC],   state->ht_vals[TJEI_LUMA_DC], TJEI_DC, 0);
    tjei_write_DHT(state, state->ht_bits[TJEI_LUMA_AC],   state->ht_vals[TJEI_LUMA_AC], TJEI_AC, 0);
    tjei_write_DHT(state, state->ht_bits[TJEI_CHROMA_DC], state->ht_vals[TJEI_CHROMA_DC], TJEI_DC, 1);
    tjei_write_DHT(state, state->ht_bits[TJEI_CHROMA_AC], state->ht_vals[TJEI_CHROMA_AC], TJEI_AC, 1);

    {
        scan_header.SOS = tjei_be_word(0xffda);
        scan_header.len = tjei_be_word((uint16_t)(6 + (sizeof(TJEFrameComponentSpec) * 3)));
        scan_header.num_components = 3;

        for (i = 0; i < 3; ++i) {
            cs.component_id = (uint8_t)(i + 1);
            cs.dc_ac = (uint8_t)scan_tables[i];
            scan_header.component_spec[i] = cs;
        }

        scan_header.first = 0;
        scan_header.last  = 63;
        scan_header.ah_al = 0;
        tjei_write(state, &scan_header, sizeof(TJEScanHeader), 1);
    }

    for ( y = 0; y < height; y += 8 ) {
        for ( x = 0; x < width; x += 8 ) {
            for ( off_y = 0; off_y < 8; ++off_y ) {
                for ( off_x = 0; off_x < 8; ++off_x ) {
                    block_index = (off_y * 8 + off_x);
                    src_index = (((y + off_y) * width) + (x + off_x)) * color_format;

                    col = x + off_x;
                    row = y + off_y;

                    if(row >= height) {
                        src_index -= (width * (row - height + 1)) * color_format;
                    }
                    if(col >= width) {
                        src_index -= (col - width + 1) * color_format;
                    }
                    assert(src_index < width * height * color_format);

                    switch (color_format)
                    {
                    case TJE_RGBA:
                        r = src_data[src_index + 0];
                        g = src_data[src_index + 1];
                        b = src_data[src_index + 2];
                        break;
                    case TJE_RGB888:
                        r = src_data[src_index + 0];
                        g = src_data[src_index + 1];
                        b = src_data[src_index + 2];
                        break;
                    case TJE_RGB565:
                        rgb = (src_data[src_index + 1] << CHAR_BIT | src_data[src_index + 0]);
                        r = (rgb & 0x001F) << 3;
                        g = (rgb & 0x07E0) >> 3;
                        b = (rgb & 0xF800) >> 8;
                        break;
                    }
                    
                    luma = 0.299f   * r + 0.587f    * g + 0.114f    * b - 128;
                    cb   = -0.1687f * r - 0.3313f   * g + 0.5f      * b;
                    cr   = 0.5f     * r - 0.4187f   * g - 0.0813f   * b;

                    du_y[block_index] = luma;
                    du_b[block_index] = cb;
                    du_r[block_index] = cr;
                }
            }

            tjei_encode_and_write_MCU(state, du_y,
                                      pqt.luma,
                                      state->ehuffsize[TJEI_LUMA_DC], state->ehuffcode[TJEI_LUMA_DC],
                                      state->ehuffsize[TJEI_LUMA_AC], state->ehuffcode[TJEI_LUMA_AC],
                                      &pred_y, &bitbuffer, &location);
            tjei_encode_and_write_MCU(state, du_b,
                                      pqt.chroma,
                                      state->ehuffsize[TJEI_CHROMA_DC], state->ehuffcode[TJEI_CHROMA_DC],
                                      state->ehuffsize[TJEI_CHROMA_AC], state->ehuffcode[TJEI_CHROMA_AC],
                                      &pred_b, &bitbuffer, &location);
            tjei_encode_and_write_MCU(state, du_r,
                                      pqt.chroma,
                                      state->ehuffsize[TJEI_CHROMA_DC], state->ehuffcode[TJEI_CHROMA_DC],
                                      state->ehuffsize[TJEI_CHROMA_AC], state->ehuffcode[TJEI_CHROMA_AC],
                                      &pred_r, &bitbuffer, &location);

        }
    }

    {
        if (location > 0 && location < 8) {
            tjei_write_bits(state, &bitbuffer, &location, (uint16_t)(8 - location), 0);
        }
    }
    EOI = tjei_be_word(0xffd9);
    tjei_write(state, &EOI, sizeof(uint16_t), 1);

    if (state->output_buffer_count) {
        state->write_context.func(state->write_context.context, state->output_buffer, (int)state->output_buffer_count);
        state->output_buffer_count = 0;
    }

    return 1;
}

static int tje_encode_with_func(tje_write_func* func, void* context, const int quality,
                                const int width, const int height, TJEColorFormat color_format, const unsigned char* src_data)
{
    TJEState state = { 0 };
    uint8_t qt_factor = 1;
    TJEWriteContext wc = { 0 };
    int i;

    if (quality < 1 || quality > 3) {
        return 0;
    }

    switch(quality) {
    case 3:
        for ( i = 0; i < 64; ++i ) {
            state.qt_luma[i]   = 1;
            state.qt_chroma[i] = 1;
        }
        break;
    case 2:
        qt_factor = 10;
    case 1:
        for ( i = 0; i < 64; ++i ) {
            state.qt_luma[i]   = tjei_default_qt_luma_from_spec[i] / qt_factor;
            if (state.qt_luma[i] == 0) {
                state.qt_luma[i] = 1;
            }
            state.qt_chroma[i] = tjei_default_qt_chroma_from_paper[i] / qt_factor;
            if (state.qt_chroma[i] == 0) {
                state.qt_chroma[i] = 1;
            }
        }
        break;
    default:
        assert(!"invalid code path");
        break;
    }

    wc.context = context;
    wc.func = func;

    state.write_context = wc;
    tjei_huff_expand(&state);
    return tjei_encode_main(&state, src_data, width, height, color_format);
}

typedef struct {
    uint8_t *jpeg_out;
    size_t *out_jpeg_len;
    size_t out_jpeg_buffer_len;
} tje_jpeg_out_param;

static void __tje_write_func(void* context, void* data, int size)
{
  tje_jpeg_out_param *param = (tje_jpeg_out_param *)context;
  assert((*param->out_jpeg_len + size) <= param->out_jpeg_buffer_len);
  memcpy(param->jpeg_out + *param->out_jpeg_len, data, size);
  *param->out_jpeg_len += size;
}

int tje_jpeg_encode(uint8_t *rgb565, int width, int height, uint8_t *out_jpeg, size_t *out_jpeg_len)
{
    int err;
    tje_jpeg_out_param param;
    param.jpeg_out = out_jpeg;
    param.out_jpeg_buffer_len = *out_jpeg_len;
    param.out_jpeg_len = out_jpeg_len;
    *out_jpeg_len = 0;

    err = tje_encode_with_func(__tje_write_func, &param, 1, width, height, TJE_RGB565, rgb565);
    if (err == 1) {
      return 0;
    }

    *out_jpeg_len = 0;
    return -1;
}

#ifdef __cplusplus
}
#endif // extern C
#endif // TJE_IMPLEMENTATION