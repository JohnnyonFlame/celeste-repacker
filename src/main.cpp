#include <cstdio>
#include <thread>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <time.h>

#include "astcenc.h"
#include "util.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static std::filesystem::path out_dir{"out"};
static std::filesystem::path gfx_dir;

static astcenc_context* astc_ctx = NULL;
static astcenc_config astc_cfg = {};

#define BLOCK_X 4
#define BLOCK_Y 4
#define ASTC_PROFILE ASTCENC_PRF_LDR
static int NUM_JOBS = 1;
static uint8_t ASTC_MAGIC[] = {0x13, 0xAB, 0xA1, 0x5C};

static const astcenc_swizzle swizzle {
    ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A
};

uint32_t get_astc_payload_length(int w, int h)
{
    unsigned int block_count_x = (w + BLOCK_X - 1) / BLOCK_X;
	unsigned int block_count_y = (h + BLOCK_Y - 1) / BLOCK_Y;

    return block_count_x * block_count_y * 16;
}

void repack(const char *filename)
{
    // Celeste texture data is stored in an RLE encoded RAW format.
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    std::filesystem::path path(filename);
    std::ifstream ifs(filename, std::ios::binary);

    int32_t width, height;
    uint8_t hasAlpha;

    ifs.read((char*)&width, sizeof(int32_t));
    ifs.read((char*)&height, sizeof(int32_t));
    ifs.read((char*)&hasAlpha, sizeof(uint8_t));

    uint8_t *imageData = new uint8_t[width * height * 4];
    for (int i = 0; i < width * height * 4; )
    {
        uint8_t repeat, r = 0, g = 0, b = 0, a = 0xFF;
        ifs.read((char*)&repeat, sizeof(uint8_t));

        if (hasAlpha)
            ifs.read((char*)&a, sizeof(uint8_t));

        // Only read color data when the run isn't transparent.
        if (a > 0)
        {
            ifs.read((char*)&r, sizeof(uint8_t));
            ifs.read((char*)&g, sizeof(uint8_t));
            ifs.read((char*)&b, sizeof(uint8_t));
        }

        while (repeat--)
        {
            imageData[i + 0] = b;
            imageData[i + 1] = g;
            imageData[i + 2] = r;
            imageData[i + 3] = a;
            i += 4;
        }
    }

    uint32_t payload_len = get_astc_payload_length(width, height);
    uint8_t *payload = new uint8_t[payload_len];

	void *slices[] = { (void*)imageData };
    astcenc_image image = {};
    image.dim_x = width;
    image.dim_y = height;
    image.dim_z = 1;
    image.data_type = ASTCENC_TYPE_U8;
    image.data = slices;

    // Spawn worker threads and wait...
    std::thread jobs[NUM_JOBS];
    for (int i = 0; i < NUM_JOBS; i++) {
        jobs[i] = std::thread([&](int job) {
            return astcenc_compress_image(astc_ctx, &image, &swizzle, payload, payload_len, job);
        }, i);
    }

    for (int i = 0; i < NUM_JOBS; i++)
        jobs[i].join();

    //astcenc_error status = astcenc_compress_image(astc_ctx, &image, &swizzle, payload, payload_len, 0);
    astcenc_error status = astcenc_compress_reset(astc_ctx);
    if (status != ASTCENC_SUCCESS)
	{
		std::printf("ERROR: Failed to compress '%s': %s\n", path.filename().c_str(), astcenc_get_error_string(status));
        delete imageData;
        delete payload;
		
        return;
	}

    std::filesystem::path fpath(filename);
    auto out_filepath = std::filesystem::proximate(fpath, gfx_dir);
    out_filepath = out_dir / out_filepath.replace_extension(".astc");

    auto out_path = out_filepath;
    out_path.remove_filename();

    // Create out path and filestream
    std::filesystem::create_directories(out_path);
    std::ofstream ofs(out_filepath, std::ios::binary);

    // write astc header
    uint8_t blks;
    ofs.write((char*)ASTC_MAGIC, sizeof(ASTC_MAGIC));
    blks = astc_cfg.block_x;        ofs.write((char*)&blks, 1);
    blks = astc_cfg.block_y;        ofs.write((char*)&blks, 1);
    blks = astc_cfg.block_z;        ofs.write((char*)&blks, 1);
    blks = ((width)        & 0xFF); ofs.write((char*)&blks, 1);
    blks = ((width  >> 8)  & 0xFF); ofs.write((char*)&blks, 1);
    blks = ((width  >> 16) & 0xFF); ofs.write((char*)&blks, 1);
    blks = ((height)       & 0xFF); ofs.write((char*)&blks, 1);
    blks = ((height >> 8)  & 0xFF); ofs.write((char*)&blks, 1);
    blks = ((height >> 16) & 0xFF); ofs.write((char*)&blks, 1);
    blks = 1;                       ofs.write((char*)&blks, 1);
    blks = 0;                       ofs.write((char*)&blks, 1);
    blks = 0;                       ofs.write((char*)&blks, 1);

    // write astc payload
    ofs.write((char*)payload, payload_len);

#if 0
    stbi_write_png("example.png", width, height, 4, imageData, width * 4);
#endif

    delete imageData;
    delete payload;

    clock_gettime(CLOCK_MONOTONIC, &end);
    std::printf("%s, res: %dx%d, alpha: %s, done in %fs\n", path.filename().c_str(), width, height, hasAlpha ? "true" : "false", diff_timespec(&start, &end) / S_IN_NS);
}

int astcenc_init()
{
    astcenc_error status;
    status = astcenc_config_init(ASTCENC_PRF_LDR, BLOCK_X, BLOCK_Y, 1, ASTC_PROFILE, 0, &astc_cfg);
    if (status != ASTCENC_SUCCESS)
    {
        std::printf("ERROR: astcenc_config_init failed: %s\n", astcenc_get_error_string(status));
        return 0;
    }

    status = astcenc_context_alloc(&astc_cfg, NUM_JOBS, &astc_ctx);
    if (status != ASTCENC_SUCCESS)
    {
        printf("ERROR: astcenc_context_alloc failed: %s\n", astcenc_get_error_string(status));
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::printf("Usage: celeste-repacker <install-dir> [--install]\n");
        std::printf("  --install Output ASTC files in the same directory.\n");
        return -1;
    }

    gfx_dir = std::filesystem::absolute(argv[1]);
    if (!std::filesystem::is_directory(gfx_dir))
    {
        std::printf("ERROR: Path is not a directory.\n");
        return -1;
    }

    // If using install parameter, then output files into the same folder
    if (argc >= 3)
    {
        if (std::string_view(argv[2]) == "--install")
        {
            out_dir = gfx_dir;
        }
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    NUM_JOBS = std::thread::hardware_concurrency();
    NUM_JOBS = NUM_JOBS ? NUM_JOBS : 1;
    if (!astcenc_init())
        return -1;

    for (auto const &entry: std::filesystem::recursive_directory_iterator{argv[1]})
    {
        if (entry.is_regular_file() && entry.path().extension() == ".data")
            repack(entry.path().c_str());
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    std::printf("Repacking finished, done in %fs\n", diff_timespec(&start, &end) / S_IN_NS);
    return 0;
}