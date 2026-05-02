// Generates a 256x256 checkerboard PNG test texture.
// Build separately or run as a one-off tool.

#include <stb_image_write.h>

#include <cstdint>
#include <cstdio>
#include <vector>

int main(int argc, char* argv[])
{
    const char* output = "assets/textures/checkerboard.png";
    if (argc > 1)
    {
        output = argv[1];
    }

    constexpr int SIZE = 256;
    constexpr int TILE = 32;
    std::vector<uint8_t> pixels(SIZE * SIZE * 4);

    for (int y = 0; y < SIZE; ++y)
    {
        for (int x = 0; x < SIZE; ++x)
        {
            bool light = ((x / TILE) + (y / TILE)) % 2 == 0;
            uint8_t c = light ? 220 : 60;
            size_t idx = (y * SIZE + x) * 4;
            pixels[idx + 0] = c;
            pixels[idx + 1] = c;
            pixels[idx + 2] = c;
            pixels[idx + 3] = 255;
        }
    }

    if (stbi_write_png(output, SIZE, SIZE, 4, pixels.data(), SIZE * 4))
    {
        std::printf("Wrote %s\n", output);
        return 0;
    }

    std::fprintf(stderr, "Failed to write %s\n", output);
    return 1;
}
