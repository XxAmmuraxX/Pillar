#include <gtest/gtest.h>
#include "Pillar/Logger.h"
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>

using namespace Pillar;

// ==============================
// Test Fixture to ensure Logger is initialized
// ==============================

class LoggerTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Ensure Logger is initialized before any tests run
        Logger::Init();
    }
};

// Register the environment
static testing::Environment* const logger_env = 
    testing::AddGlobalTestEnvironment(new LoggerTestEnvironment);

// ==============================
// Logger Initialization Tests
// ==============================

TEST(LoggerTests, Logger_IsInitialized) {
    // Logger should be initialized by the time tests run
    EXPECT_NE(Logger::GetCoreLogger(), nullptr);
    EXPECT_NE(Logger::GetClientLogger(), nullptr);
}

TEST(LoggerTests, Logger_CoreLoggerName) {
    auto logger = Logger::GetCoreLogger();
    EXPECT_EQ(logger->name(), "Pillar");
}

TEST(LoggerTests, Logger_ClientLoggerName) {
    auto logger = Logger::GetClientLogger();
    EXPECT_EQ(logger->name(), "Client");
}

// ==============================
// Logger Output Tests (using custom sink)
// ==============================

class LoggerOutputTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Save original loggers
        m_OriginalCoreLogger = Logger::GetCoreLogger();
        m_OriginalClientLogger = Logger::GetClientLogger();
        
        // Create test loggers with string stream sinks
        m_CoreStream = std::make_shared<std::ostringstream>();
        m_ClientStream = std::make_shared<std::ostringstream>();
        
        auto coreSink = std::make_shared<spdlog::sinks::ostream_sink_mt>(*m_CoreStream);
        auto clientSink = std::make_shared<spdlog::sinks::ostream_sink_mt>(*m_ClientStream);
        
        m_TestCoreLogger = std::make_shared<spdlog::logger>("TEST_CORE", coreSink);
        m_TestClientLogger = std::make_shared<spdlog::logger>("TEST_CLIENT", clientSink);
        
        m_TestCoreLogger->set_level(spdlog::level::trace);
        m_TestClientLogger->set_level(spdlog::level::trace);
        
        m_TestCoreLogger->set_pattern("%v");
        m_TestClientLogger->set_pattern("%v");
    }
    
    void TearDown() override {
        // Restore original loggers would go here if Logger had setters
    }
    
    std::shared_ptr<spdlog::logger> m_OriginalCoreLogger;
    std::shared_ptr<spdlog::logger> m_OriginalClientLogger;
    std::shared_ptr<spdlog::logger> m_TestCoreLogger;
    std::shared_ptr<spdlog::logger> m_TestClientLogger;
    std::shared_ptr<std::ostringstream> m_CoreStream;
    std::shared_ptr<std::ostringstream> m_ClientStream;
};

TEST_F(LoggerOutputTest, Logger_TraceLevel) {
    m_TestCoreLogger->trace("Test trace message");
    EXPECT_NE(m_CoreStream->str().find("Test trace message"), std::string::npos);
}

TEST_F(LoggerOutputTest, Logger_InfoLevel) {
    m_TestCoreLogger->info("Test info message");
    EXPECT_NE(m_CoreStream->str().find("Test info message"), std::string::npos);
}

TEST_F(LoggerOutputTest, Logger_WarnLevel) {
    m_TestCoreLogger->warn("Test warn message");
    EXPECT_NE(m_CoreStream->str().find("Test warn message"), std::string::npos);
}

TEST_F(LoggerOutputTest, Logger_ErrorLevel) {
    m_TestCoreLogger->error("Test error message");
    EXPECT_NE(m_CoreStream->str().find("Test error message"), std::string::npos);
}

TEST_F(LoggerOutputTest, Logger_FormattedMessage) {
    m_TestCoreLogger->info("Value: {0}", 42);
    EXPECT_NE(m_CoreStream->str().find("Value: 42"), std::string::npos);
}

TEST_F(LoggerOutputTest, Logger_MultipleArguments) {
    m_TestCoreLogger->info("X: {0}, Y: {1}", 10, 20);
    std::string output = m_CoreStream->str();
    EXPECT_NE(output.find("X: 10"), std::string::npos);
    EXPECT_NE(output.find("Y: 20"), std::string::npos);
}

// ==============================
// Logger Level Tests
// ==============================

TEST_F(LoggerOutputTest, Logger_LevelFiltering) {
    m_TestCoreLogger->set_level(spdlog::level::warn);
    
    m_TestCoreLogger->trace("Should not appear");
    m_TestCoreLogger->info("Should not appear");
    m_TestCoreLogger->warn("Should appear");
    
    std::string output = m_CoreStream->str();
    EXPECT_EQ(output.find("Should not appear"), std::string::npos);
    EXPECT_NE(output.find("Should appear"), std::string::npos);
}

// ==============================
// Basic Macro Tests (compile-time check)
// ==============================

TEST(LoggerMacroTests, CoreMacros_Compile) {
    // These should compile without errors
    PIL_CORE_TRACE("Trace");
    PIL_CORE_INFO("Info");
    PIL_CORE_WARN("Warn");
    PIL_CORE_ERROR("Error");
}

TEST(LoggerMacroTests, ClientMacros_Compile) {
    // These should compile without errors
    PIL_TRACE("Trace");
    PIL_INFO("Info");
    PIL_WARN("Warn");
    PIL_ERROR("Error");
}

TEST(LoggerMacroTests, FormattedMacros_Compile) {
    // These should compile without errors
    int value = 42;
    PIL_CORE_INFO("Value: {0}", value);
    PIL_INFO("Value: {0}", value);
}
