#include <gtest/gtest.h>
#include "core/config.h"
#include "core/logging.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class ConfigTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        placeholder::core::initLogging();
        m_tempDir = fs::temp_directory_path() / "placeholder_tests";
        fs::create_directories(m_tempDir);
    }

    void TearDown() override
    {
        fs::remove_all(m_tempDir);
    }

    fs::path writeJson(const std::string& filename, const std::string& content)
    {
        auto path = m_tempDir / filename;
        std::ofstream file(path);
        file << content;
        return path;
    }

    fs::path m_tempDir;
};

TEST_F(ConfigTest, LoadValidFile)
{
    auto path = writeJson("valid.json", R"({
        "window": { "width": 1920, "height": 1080 },
        "rendering": { "vsync": true }
    })");

    placeholder::core::Config config;
    EXPECT_TRUE(config.loadFromFile(path));
    EXPECT_EQ(config.get<int>("window.width"), 1920);
    EXPECT_EQ(config.get<int>("window.height"), 1080);
    EXPECT_EQ(config.get<bool>("rendering.vsync"), true);
}

TEST_F(ConfigTest, LoadNonExistentFile)
{
    placeholder::core::Config config;
    EXPECT_FALSE(config.loadFromFile(m_tempDir / "missing.json"));
}

TEST_F(ConfigTest, LoadInvalidJson)
{
    auto path = writeJson("invalid.json", "{ not valid json }}}");
    placeholder::core::Config config;
    EXPECT_FALSE(config.loadFromFile(path));
}

TEST_F(ConfigTest, GetWithDefault)
{
    placeholder::core::Config config;
    EXPECT_EQ(config.get<int>("nonexistent.key", 42), 42);
    EXPECT_EQ(config.get<std::string>("missing", "fallback"), "fallback");
}

TEST_F(ConfigTest, SetAndGet)
{
    placeholder::core::Config config;
    config.set("window.width", 2560);
    config.set("window.title", std::string("Test"));
    config.set("rendering.vsync", false);

    EXPECT_EQ(config.get<int>("window.width"), 2560);
    EXPECT_EQ(config.get<std::string>("window.title"), "Test");
    EXPECT_EQ(config.get<bool>("rendering.vsync"), false);
}

TEST_F(ConfigTest, HasKeyPath)
{
    placeholder::core::Config config;
    config.set("a.b.c", 1);

    EXPECT_TRUE(config.has("a"));
    EXPECT_TRUE(config.has("a.b"));
    EXPECT_TRUE(config.has("a.b.c"));
    EXPECT_FALSE(config.has("a.b.d"));
    EXPECT_FALSE(config.has("x"));
}

TEST_F(ConfigTest, SaveAndReload)
{
    auto path = m_tempDir / "roundtrip.json";

    placeholder::core::Config config;
    config.set("game.name", std::string("Placeholder"));
    config.set("game.version", 1);
    EXPECT_TRUE(config.saveToFile(path));

    placeholder::core::Config reloaded;
    EXPECT_TRUE(reloaded.loadFromFile(path));
    EXPECT_EQ(reloaded.get<std::string>("game.name"), "Placeholder");
    EXPECT_EQ(reloaded.get<int>("game.version"), 1);
}

TEST_F(ConfigTest, SaveWithNoPath)
{
    placeholder::core::Config config;
    EXPECT_FALSE(config.saveToFile());
}

TEST_F(ConfigTest, CommandLineOverrideExistingInt)
{
    auto path = writeJson("cli.json", R"({ "window": { "width": 1920 } })");

    placeholder::core::Config config;
    config.loadFromFile(path);

    const char* argv[] = {"app", "--window.width=2560"};
    config.applyCommandLine(2, const_cast<char**>(argv));

    EXPECT_EQ(config.get<int>("window.width"), 2560);
}

TEST_F(ConfigTest, CommandLineOverrideExistingBool)
{
    auto path = writeJson("cli_bool.json", R"({ "rendering": { "vsync": true } })");

    placeholder::core::Config config;
    config.loadFromFile(path);

    const char* argv[] = {"app", "--rendering.vsync=false"};
    config.applyCommandLine(2, const_cast<char**>(argv));

    EXPECT_EQ(config.get<bool>("rendering.vsync"), false);
}

TEST_F(ConfigTest, CommandLineNewKey)
{
    placeholder::core::Config config;
    const char* argv[] = {"app", "--custom.key=hello"};
    config.applyCommandLine(2, const_cast<char**>(argv));

    EXPECT_EQ(config.get<std::string>("custom.key"), "hello");
}

TEST_F(ConfigTest, CommandLineIgnoresNonDashArgs)
{
    placeholder::core::Config config;
    const char* argv[] = {"app", "positional", "-single", "--noequals"};
    config.applyCommandLine(4, const_cast<char**>(argv));

    EXPECT_FALSE(config.has("positional"));
    EXPECT_FALSE(config.has("single"));
    EXPECT_FALSE(config.has("noequals"));
}

TEST_F(ConfigTest, RawJsonAccess)
{
    auto path = writeJson("raw.json", R"({ "key": "value" })");

    placeholder::core::Config config;
    config.loadFromFile(path);

    EXPECT_TRUE(config.raw().is_object());
    EXPECT_EQ(config.raw()["key"], "value");
}

TEST_F(ConfigTest, TypeMismatchReturnsDefault)
{
    placeholder::core::Config config;
    config.set("num", 42);

    EXPECT_EQ(config.get<std::string>("num", "fallback"), "fallback");
}
