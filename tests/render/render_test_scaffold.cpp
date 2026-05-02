#include <gtest/gtest.h>
#include "core/logging.h"

/// Render test scaffold.
///
/// These tests require an OpenGL context (GLFW window), which makes them
/// environment-dependent. The scaffold below provides the FBO-based comparison
/// framework; actual render tests will be added as rendering features mature.
///
/// To run: create a hidden GLFW window, render to an offscreen FBO, read back
/// pixels with glReadPixels, and compare against reference PNGs in tests/data/.
///
/// For now this file contains only a placeholder test so the target compiles.
/// The real render comparison utilities will be added when Phase 6 (Asset
/// Pipeline) provides texture loading for reading reference images.

namespace
{

/// Compute the mean squared error between two RGBA pixel buffers.
double computeMSE(const uint8_t* a, const uint8_t* b, size_t pixelCount)
{
    double sum = 0.0;
    size_t channels = pixelCount * 4;
    for (size_t i = 0; i < channels; ++i)
    {
        double diff = static_cast<double>(a[i]) - static_cast<double>(b[i]);
        sum += diff * diff;
    }
    return sum / static_cast<double>(channels);
}

} // namespace

TEST(RenderTestScaffold, MSEIdenticalImages)
{
    const size_t pixels = 64;
    std::vector<uint8_t> image(pixels * 4, 128);

    double mse = computeMSE(image.data(), image.data(), pixels);
    EXPECT_DOUBLE_EQ(mse, 0.0);
}

TEST(RenderTestScaffold, MSEDifferentImages)
{
    const size_t pixels = 4;
    std::vector<uint8_t> a(pixels * 4, 0);
    std::vector<uint8_t> b(pixels * 4, 10);

    double mse = computeMSE(a.data(), b.data(), pixels);
    EXPECT_DOUBLE_EQ(mse, 100.0);
}

TEST(RenderTestScaffold, MSEThresholdCheck)
{
    const size_t pixels = 16;
    std::vector<uint8_t> a(pixels * 4, 100);
    std::vector<uint8_t> b(pixels * 4, 101);

    double mse = computeMSE(a.data(), b.data(), pixels);
    EXPECT_LT(mse, 2.0);
}
