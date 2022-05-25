#pragma once

#define BLOCK_X 4
#define BLOCK_Y 4
#define ASTC_QUALITY ASTCENC_PRE_FAST

struct astc_header {
    union {
        struct {
            uint8_t magic[4];
            uint8_t block_x;
            uint8_t block_y;
            uint8_t block_z;
            unsigned int dim_x: 24;
            unsigned int dim_y: 24;
            unsigned int dim_z: 24;
        } __attribute__((packed));
        char data[16];
    };
};