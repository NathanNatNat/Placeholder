#include <gtest/gtest.h>
#include "core/console.h"
#include "core/logging.h"

using namespace placeholder::core;

class ConsoleTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        initLogging();
    }

    Console console;
};

TEST_F(ConsoleTest, RegisterAndExecuteCommand)
{
    console.registerCommand("greet", "Say hello", [](const std::vector<std::string>&)
    {
        return "Hello!";
    });

    std::string result = console.execute("greet");
    EXPECT_EQ(result, "Hello!");
}

TEST_F(ConsoleTest, CommandReceivesArguments)
{
    console.registerCommand("echo", "Echo args", [](const std::vector<std::string>& args)
    {
        std::string result;
        for (const auto& arg : args)
        {
            if (!result.empty()) result += " ";
            result += arg;
        }
        return result;
    });

    EXPECT_EQ(console.execute("echo one two three"), "one two three");
}

TEST_F(ConsoleTest, QuotedArgumentsAreGrouped)
{
    console.registerCommand("echo", "Echo args", [](const std::vector<std::string>& args)
    {
        return std::to_string(args.size()) + ":" + (args.empty() ? "" : args[0]);
    });

    EXPECT_EQ(console.execute("echo \"hello world\""), "1:hello world");
}

TEST_F(ConsoleTest, UnknownCommandReturnsError)
{
    std::string result = console.execute("nonexistent");
    EXPECT_NE(result.find("Unknown command"), std::string::npos);
}

TEST_F(ConsoleTest, EmptyInputReturnsEmpty)
{
    EXPECT_TRUE(console.execute("").empty());
    EXPECT_TRUE(console.execute("   ").empty());
}

TEST_F(ConsoleTest, CaseInsensitiveLookup)
{
    console.registerCommand("wireframe", "Toggle wireframe", [](const std::vector<std::string>&)
    {
        return "toggled";
    });

    EXPECT_EQ(console.execute("wireframe"), "toggled");
    EXPECT_EQ(console.execute("WIREFRAME"), "toggled");
    EXPECT_EQ(console.execute("Wireframe"), "toggled");
}

TEST_F(ConsoleTest, HistoryTracking)
{
    console.registerCommand("test", "Test cmd", [](const std::vector<std::string>&)
    {
        return "ok";
    });

    console.execute("test a");
    console.execute("test b");

    const auto& history = console.history();
    ASSERT_EQ(history.size(), 2u);
    EXPECT_EQ(history[0], "test a");
    EXPECT_EQ(history[1], "test b");
}

TEST_F(ConsoleTest, OutputLogContainsInputAndOutput)
{
    console.registerCommand("ping", "Ping", [](const std::vector<std::string>&)
    {
        return "pong";
    });

    console.execute("ping");

    const auto& log = console.outputLog();
    ASSERT_GE(log.size(), 2u);
    EXPECT_EQ(log[0], "> ping");
    EXPECT_EQ(log[1], "pong");
}

TEST_F(ConsoleTest, ClearOutput)
{
    console.registerCommand("cmd", "Test", [](const std::vector<std::string>&)
    {
        return "result";
    });

    console.execute("cmd");
    EXPECT_FALSE(console.outputLog().empty());

    console.clearOutput();
    EXPECT_TRUE(console.outputLog().empty());
}

TEST_F(ConsoleTest, AutocompleteMatchesPrefix)
{
    console.registerCommand("wireframe", "Toggle wireframe", [](const std::vector<std::string>&) { return ""; });
    console.registerCommand("wirecolor", "Wire color", [](const std::vector<std::string>&) { return ""; });
    console.registerCommand("stats", "Show stats", [](const std::vector<std::string>&) { return ""; });

    auto matches = console.autocomplete("wire");
    ASSERT_EQ(matches.size(), 2u);
    EXPECT_EQ(matches[0], "wirecolor");
    EXPECT_EQ(matches[1], "wireframe");
}

TEST_F(ConsoleTest, AutocompleteNoMatch)
{
    console.registerCommand("test", "Test", [](const std::vector<std::string>&) { return ""; });

    auto matches = console.autocomplete("xyz");
    EXPECT_TRUE(matches.empty());
}

TEST_F(ConsoleTest, HelpTextForSpecificCommand)
{
    console.registerCommand("reload", "Reload all shaders", [](const std::vector<std::string>&) { return ""; });

    std::string help = console.helpText("reload");
    EXPECT_NE(help.find("reload"), std::string::npos);
    EXPECT_NE(help.find("Reload all shaders"), std::string::npos);
}

TEST_F(ConsoleTest, HelpTextListsAllCommands)
{
    console.registerCommand("alpha", "First", [](const std::vector<std::string>&) { return ""; });
    console.registerCommand("beta", "Second", [](const std::vector<std::string>&) { return ""; });

    std::string help = console.helpText();
    EXPECT_NE(help.find("alpha"), std::string::npos);
    EXPECT_NE(help.find("beta"), std::string::npos);
}

TEST_F(ConsoleTest, HelpTextUnknownCommand)
{
    std::string help = console.helpText("missing");
    EXPECT_NE(help.find("Unknown command"), std::string::npos);
}

TEST_F(ConsoleTest, CallbackExceptionIsCaught)
{
    console.registerCommand("crash", "Throws", [](const std::vector<std::string>&) -> std::string
    {
        throw std::runtime_error("boom");
    });

    std::string result = console.execute("crash");
    EXPECT_NE(result.find("Error"), std::string::npos);
    EXPECT_NE(result.find("boom"), std::string::npos);
}
