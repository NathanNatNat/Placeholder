#include <gtest/gtest.h>
#include "core/logging.h"
#include "core/config.h"
#include "core/console.h"
#include "core/file_io.h"

#include <filesystem>

namespace fs = std::filesystem;
using namespace placeholder::core;

class IntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        initLogging();
        m_tempDir = fs::temp_directory_path() / "placeholder_integration";
        fs::create_directories(m_tempDir);
    }

    void TearDown() override
    {
        fs::remove_all(m_tempDir);
    }

    fs::path m_tempDir;
};

TEST_F(IntegrationTest, ConfigRoundTripThroughFileIO)
{
    Config config;
    config.set("window.width", 2560);
    config.set("window.height", 1440);
    config.set("rendering.vsync", true);

    auto path = m_tempDir / "config.json";
    ASSERT_TRUE(config.saveToFile(path));

    auto text = readFileText(path);
    ASSERT_TRUE(text.has_value());
    EXPECT_NE(text->find("2560"), std::string::npos);

    Config reloaded;
    ASSERT_TRUE(reloaded.loadFromFile(path));
    EXPECT_EQ(reloaded.get<int>("window.width"), 2560);
    EXPECT_EQ(reloaded.get<int>("window.height"), 1440);
    EXPECT_EQ(reloaded.get<bool>("rendering.vsync"), true);
}

TEST_F(IntegrationTest, ConsoleWithConfigCommands)
{
    Config config;
    config.set("rendering.wireframe", false);

    Console console;
    console.registerCommand("wireframe", "Toggle wireframe rendering",
        [&config](const std::vector<std::string>& args) -> std::string
        {
            if (args.empty())
            {
                bool current = config.get<bool>("rendering.wireframe");
                config.set("rendering.wireframe", !current);
                return !current ? "Wireframe ON" : "Wireframe OFF";
            }
            bool value = (args[0] == "on" || args[0] == "true" || args[0] == "1");
            config.set("rendering.wireframe", value);
            return value ? "Wireframe ON" : "Wireframe OFF";
        });

    EXPECT_EQ(console.execute("wireframe"), "Wireframe ON");
    EXPECT_TRUE(config.get<bool>("rendering.wireframe"));

    EXPECT_EQ(console.execute("wireframe off"), "Wireframe OFF");
    EXPECT_FALSE(config.get<bool>("rendering.wireframe"));
}
