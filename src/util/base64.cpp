﻿/**
 * @file
 * @brief Base64 encode/decode
 * @author simon
 */

#include <cstdio>
#include "base64.h"
#include <memory>
#include <climits>
/* ---------------- private code */
static const uint8_t map2[] =
{
    0x3e, 0xff, 0xff, 0xff, 0x3f, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1a, 0x1b,
    0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
    0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33
};

int av_base64_decode(uint8_t *out, const char *in, int out_size)
{
    int i, v;
    uint8_t *dst = out;

    v = 0;
    for (i = 0; in[i] && in[i] != '='; i++) {
        unsigned int index= in[i]-43;
        if (index>=FF_ARRAY_ELEMS(map2) || map2[index] == 0xff)
            return -1;
        v = (v << 6) + map2[index];
        if (i & 3) {
            if (dst - out < out_size) {
                *dst++ = v >> (6 - 2 * (i & 3));
            }
        }
    }

    return dst - out;
}

/*****************************************************************************
* b64_encode: Stolen from VLC's http.c.
* Simplified by Michael.
* Fixed edge cases and made it work from data (vs. strings) by Ryan.
*****************************************************************************/

char *av_base64_encode(char *out, int out_size, const uint8_t *in, int in_size)
{
    static const char b64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *ret, *dst;
    unsigned i_bits = 0;
    int i_shift = 0;
    int bytes_remaining = in_size;

    if (in_size >= UINT_MAX / 4 || out_size < AV_BASE64_SIZE(in_size))
     {
         return nullptr;
     }
    ret = dst = out;
    while (bytes_remaining) {
        i_bits = (i_bits << unsigned(8)) + *in++;
        bytes_remaining--;
        i_shift += 8;

        do {
            *dst++ = b64[(i_bits << 6 >> i_shift) & 0x3f];
            i_shift -= 6;
        } while (i_shift > 6 || (bytes_remaining == 0 && i_shift > 0));
    }
    while ((dst - ret) & 3)
        *dst++ = '=';
    *dst = '\0';

    return ret;
}

string encodeBase64(const string &txt){
    if(txt.empty()){
        return "";
    }
    int size = AV_BASE64_SIZE(txt.size()) + 10;
    std::shared_ptr<char> txt_enc(new char[size],[](const char *ptr){delete [] ptr;});
    auto ret = av_base64_encode(txt_enc.get(),size,(uint8_t *)txt.data(),txt.size());
    if(!ret ){
        return "";
    }
    return ret;
}

string decodeBase64(const string &txt){
    if(txt.empty()){
        return "";
    }
    int size = txt.size() * 3 / 4 +10;
    std::shared_ptr<char> txt_dec(new char[size],[](const char *ptr){delete [] ptr;});
    size = av_base64_decode((uint8_t *)txt_dec.get(),txt.data(),size);
    if(size <= 0){
        return "";
    }
    return string(txt_dec.get(),size);
}

#ifdef TEST

#undef printf

#define MAX_DATA_SIZE    1024
#define MAX_ENCODED_SIZE 2048

static int test_encode_decode(const uint8_t *data, unsigned int data_size,
                              const char *encoded_ref)
{
    char  encoded[MAX_ENCODED_SIZE];
    uint8_t data2[MAX_DATA_SIZE];
    int data2_size, max_data2_size = MAX_DATA_SIZE;

    if (!av_base64_encode(encoded, MAX_ENCODED_SIZE, data, data_size)) {
        printf("Failed: cannot encode the input data\n");
        return 1;
    }
    if (encoded_ref && strcmp(encoded, encoded_ref)) {
        printf("Failed: encoded string differs from reference\n"
               "Encoded:\n%s\nReference:\n%s\n", encoded, encoded_ref);
        return 1;
    }

    if ((data2_size = av_base64_decode(data2, encoded, max_data2_size)) < 0) {
        printf("Failed: cannot decode the encoded string\n"
               "Encoded:\n%s\n", encoded);
        return 1;
    }
    if (memcmp(data2, data, data_size)) {
        printf("Failed: encoded/decoded data differs from original data\n");
        return 1;
    }

    printf("Passed!\n");
    return 0;
}

int main(void)
{
    int i, error_count = 0;
    struct test {
        const uint8_t *data;
        const char *encoded_ref;
    } tests[] = {
        { "",        ""},
        { "1",       "MQ=="},
        { "22",      "MjI="},
        { "333",     "MzMz"},
        { "4444",    "NDQ0NA=="},
        { "55555",   "NTU1NTU="},
        { "666666",  "NjY2NjY2"},
        { "abc:def", "YWJjOmRlZg=="},
    };

    printf("Encoding/decoding tests\n");
    for (i = 0; i < FF_ARRAY_ELEMS(tests); i++)
        error_count += test_encode_decode(tests[i].data, strlen(tests[i].data), tests[i].encoded_ref);

    return error_count;
}

#endif
