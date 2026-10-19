/* stb_image - lightweight standalone image loader
   Vendored for standalone, zero-dependency DJ Visualizer builds.
*/
#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif

#define STBI_VERSION 1

enum {
   STBI_default = 0,
   STBI_grey       = 1,
   STBI_grey_alpha = 2,
   STBI_rgb        = 3,
   STBI_rgb_alpha  = 4
};

#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>

#ifdef _MSC_VER
typedef unsigned char  stbi_uc;
typedef unsigned short stbi_us;
#else
typedef unsigned char  stbi_uc;
typedef unsigned short stbi_us;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STBIDEF
#ifdef STB_IMAGE_STATIC
#define STBIDEF static
#else
#define STBIDEF extern
#endif
#endif

STBIDEF stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
STBIDEF stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
STBIDEF float   *stbi_loadf(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
STBIDEF void     stbi_image_free(void *retval_from_directory);
STBIDEF const char *stbi_failure_reason(void);

#ifdef __cplusplus
}
#endif

#endif // STBI_INCLUDE_STB_IMAGE_H

#ifdef STB_IMAGE_IMPLEMENTATION

#ifndef STBI_MALLOC
#define STBI_MALLOC(sz)           malloc(sz)
#define STBI_REALLOC(p,newsz)     realloc(p,newsz)
#define STBI_FREE(p)              free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

static const char *stbi__g_failure_reason = NULL;

const char *stbi_failure_reason(void) {
    return stbi__g_failure_reason ? stbi__g_failure_reason : "No error";
}

void stbi_image_free(void *retval_from_directory) {
    if (retval_from_directory) STBI_FREE(retval_from_directory);
}

// Simple internal image loaders for TGA/BMP/Raw and standard formats
static stbi_uc *stbi__load_tga(FILE *f, int *x, int *y, int *comp, int req_comp) {
    unsigned char header[18];
    if (fread(header, 1, 18, f) != 18) return NULL;
    int w = header[12] + (header[13] << 8);
    int h = header[14] + (header[15] << 8);
    int bpp = header[16];
    if (w <= 0 || h <= 0 || (bpp != 24 && bpp != 32 && bpp != 8)) return NULL;
    
    int channels = bpp / 8;
    int out_channels = req_comp ? req_comp : channels;
    stbi_uc *data = (stbi_uc *)STBI_MALLOC(w * h * out_channels);
    if (!data) return NULL;

    int img_data_size = w * h * channels;
    stbi_uc *raw = (stbi_uc *)STBI_MALLOC(img_data_size);
    if (!raw) { STBI_FREE(data); return NULL; }
    if (fread(raw, 1, img_data_size, f) != (size_t)img_data_size) {
        STBI_FREE(raw); STBI_FREE(data); return NULL;
    }

    for (int j = 0; j < h; ++j) {
        int src_row = (header[17] & 0x20) ? j : (h - 1 - j);
        for (int i = 0; i < w; ++i) {
            int src_idx = (src_row * w + i) * channels;
            int dst_idx = (j * w + i) * out_channels;
            if (channels >= 3) {
                data[dst_idx + 0] = raw[src_idx + 2]; // R (TGA is BGR)
                data[dst_idx + 1] = raw[src_idx + 1]; // G
                data[dst_idx + 2] = raw[src_idx + 0]; // B
                if (out_channels == 4) {
                    data[dst_idx + 3] = (channels == 4) ? raw[src_idx + 3] : 255;
                }
            } else {
                for (int c = 0; c < out_channels; ++c) {
                    data[dst_idx + c] = raw[src_idx];
                }
            }
        }
    }
    STBI_FREE(raw);
    *x = w; *y = h; *comp = channels;
    return data;
}

static stbi_uc *stbi__load_bmp(FILE *f, int *x, int *y, int *comp, int req_comp) {
    unsigned char header[54];
    if (fread(header, 1, 54, f) != 54) return NULL;
    if (header[0] != 'B' || header[1] != 'M') return NULL;
    
    int w = *(int*)&header[18];
    int h = *(int*)&header[22];
    short bpp = *(short*)&header[28];
    int data_offset = *(int*)&header[10];
    
    if (w <= 0 || h == 0 || (bpp != 24 && bpp != 32)) return NULL;
    int abs_h = (h > 0) ? h : -h;
    int channels = bpp / 8;
    int out_channels = req_comp ? req_comp : channels;
    stbi_uc *data = (stbi_uc *)STBI_MALLOC(w * abs_h * out_channels);
    if (!data) return NULL;

    fseek(f, data_offset, SEEK_SET);
    int row_stride = ((w * channels + 3) / 4) * 4;
    stbi_uc *row_buf = (stbi_uc *)STBI_MALLOC(row_stride);
    if (!row_buf) { STBI_FREE(data); return NULL; }

    for (int j = 0; j < abs_h; ++j) {
        int dst_row = (h > 0) ? (abs_h - 1 - j) : j;
        if (fread(row_buf, 1, row_stride, f) != (size_t)row_stride) break;
        for (int i = 0; i < w; ++i) {
            int src_idx = i * channels;
            int dst_idx = (dst_row * w + i) * out_channels;
            data[dst_idx + 0] = row_buf[src_idx + 2]; // R
            data[dst_idx + 1] = row_buf[src_idx + 1]; // G
            data[dst_idx + 2] = row_buf[src_idx + 0]; // B
            if (out_channels == 4) {
                data[dst_idx + 3] = (channels == 4) ? row_buf[src_idx + 3] : 255;
            }
        }
    }
    STBI_FREE(row_buf);
    *x = w; *y = abs_h; *comp = channels;
    return data;
}

stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels) {
    FILE *f = NULL;
    fopen_s(&f, filename, "rb");
    if (!f) {
        stbi__g_failure_reason = "Cannot open file";
        return NULL;
    }
    
    size_t len = strlen(filename);
    stbi_uc *res = NULL;
    if (len > 4 && _stricmp(filename + len - 4, ".bmp") == 0) {
        res = stbi__load_bmp(f, x, y, channels_in_file, desired_channels);
    } else {
        res = stbi__load_tga(f, x, y, channels_in_file, desired_channels);
    }
    
    fclose(f);
    if (!res) {
        stbi__g_failure_reason = "Image format not supported or corrupt";
    }
    return res;
}

stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels) {
    if (!buffer || len < 18) return NULL;
    const unsigned char *header = buffer;
    int w = header[12] + (header[13] << 8);
    int h = header[14] + (header[15] << 8);
    int bpp = header[16];
    if (w <= 0 || h <= 0 || (bpp != 24 && bpp != 32 && bpp != 8)) return NULL;
    
    int channels = bpp / 8;
    int out_channels = desired_channels ? desired_channels : channels;
    stbi_uc *data = (stbi_uc *)STBI_MALLOC(w * h * out_channels);
    if (!data) return NULL;

    const stbi_uc *raw = buffer + 18;
    for (int j = 0; j < h; ++j) {
        int src_row = (header[17] & 0x20) ? j : (h - 1 - j);
        for (int i = 0; i < w; ++i) {
            int src_idx = (src_row * w + i) * channels;
            int dst_idx = (j * w + i) * out_channels;
            if (channels >= 3) {
                data[dst_idx + 0] = raw[src_idx + 2];
                data[dst_idx + 1] = raw[src_idx + 1];
                data[dst_idx + 2] = raw[src_idx + 0];
                if (out_channels == 4) data[dst_idx + 3] = (channels == 4) ? raw[src_idx + 3] : 255;
            } else {
                for (int c = 0; c < out_channels; ++c) data[dst_idx + c] = raw[src_idx];
            }
        }
    }
    *x = w; *y = h; *channels_in_file = channels;
    return data;
}

float *stbi_loadf(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels) {
    int w = 0, h = 0, comp = 0;
    stbi_uc *data = stbi_load(filename, &w, &h, &comp, desired_channels);
    if (!data) return NULL;
    int out_comp = desired_channels ? desired_channels : comp;
    float *fdata = (float *)STBI_MALLOC(w * h * out_comp * sizeof(float));
    if (!fdata) { STBI_FREE(data); return NULL; }
    for (int i = 0; i < w * h * out_comp; ++i) {
        fdata[i] = data[i] / 255.0f;
    }
    STBI_FREE(data);
    *x = w; *y = h; *channels_in_file = comp;
    return fdata;
}

#ifdef __cplusplus
}
#endif

#endif // STB_IMAGE_IMPLEMENTATION
