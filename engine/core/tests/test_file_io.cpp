#include <gtest/gtest.h>
#include "core/file_io.h"
#include "core/logging.h"

#include <filesystem>

namespace fs = std::filesystem;
using namespace placeholder::core;

class FileIOTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        initLogging();
        m_tempDir = fs::temp_directory_path() / "placeholder_fileio_tests";
        fs::create_directories(m_tempDir);
    }

    void TearDown() override
    {
        fs::remove_all(m_tempDir);
    }

    fs::path m_tempDir;
};

TEST_F(FileIOTest, WriteAndReadText)
{
    auto path = m_tempDir / "hello.txt";
    EXPECT_TRUE(writeFileText(path, "Hello, World!"));

    auto content = readFileText(path);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "Hello, World!");
}

TEST_F(FileIOTest, WriteAndReadBinary)
{
    auto path = m_tempDir / "data.bin";
    std::vector<uint8_t> data = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD};
    EXPECT_TRUE(writeFileBinary(path, data.data(), data.size()));

    auto content = readFileBinary(path);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, data);
}

TEST_F(FileIOTest, ReadNonExistentTextFile)
{
    auto result = readFileText(m_tempDir / "missing.txt");
    EXPECT_FALSE(result.has_value());
}

TEST_F(FileIOTest, ReadNonExistentBinaryFile)
{
    auto result = readFileBinary(m_tempDir / "missing.bin");
    EXPECT_FALSE(result.has_value());
}

TEST_F(FileIOTest, WriteCreatesParentDirectories)
{
    auto path = m_tempDir / "nested" / "deep" / "file.txt";
    EXPECT_TRUE(writeFileText(path, "nested content"));

    auto content = readFileText(path);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "nested content");
}

TEST_F(FileIOTest, WriteEmptyFile)
{
    auto path = m_tempDir / "empty.txt";
    EXPECT_TRUE(writeFileText(path, ""));

    auto content = readFileText(path);
    ASSERT_TRUE(content.has_value());
    EXPECT_TRUE(content->empty());
}

TEST_F(FileIOTest, WriteBinaryCreatesParentDirectories)
{
    auto path = m_tempDir / "sub" / "dir" / "data.bin";
    uint8_t byte = 0x42;
    EXPECT_TRUE(writeFileBinary(path, &byte, 1));

    auto content = readFileBinary(path);
    ASSERT_TRUE(content.has_value());
    ASSERT_EQ(content->size(), 1u);
    EXPECT_EQ((*content)[0], 0x42);
}

TEST_F(FileIOTest, GetExtensionBasicCases)
{
    EXPECT_EQ(getExtension("model.m2"), "m2");
    EXPECT_EQ(getExtension("texture.BLP"), "blp");
    EXPECT_EQ(getExtension("archive.tar.gz"), "gz");
    EXPECT_EQ(getExtension("noext"), "");
}

TEST_F(FileIOTest, GetExtensionWithPath)
{
    EXPECT_EQ(getExtension(fs::path("C:/data/models/character.M2")), "m2");
    EXPECT_EQ(getExtension(fs::path("/tmp/test.JSON")), "json");
}

TEST_F(FileIOTest, NormalizePath)
{
    auto normalized = normalizePath(fs::path("a/b/../c/./d"));
    EXPECT_EQ(normalized.generic_string(), "a/c/d");
}

TEST_F(FileIOTest, OverwriteExistingFile)
{
    auto path = m_tempDir / "overwrite.txt";
    EXPECT_TRUE(writeFileText(path, "first"));
    EXPECT_TRUE(writeFileText(path, "second"));

    auto content = readFileText(path);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "second");
}

TEST_F(FileIOTest, LargerBinaryRoundTrip)
{
    auto path = m_tempDir / "large.bin";
    std::vector<uint8_t> data(64 * 1024);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<uint8_t>(i & 0xFF);
    }

    EXPECT_TRUE(writeFileBinary(path, data.data(), data.size()));

    auto content = readFileBinary(path);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, data);
}
