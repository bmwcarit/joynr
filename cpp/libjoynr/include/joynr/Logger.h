/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#ifndef LOGGER_H
#define LOGGER_H

#pragma GCC diagnostic push
#if defined(__GNUC__) && (__GNUC___ > 5 || (__GNUC__ == 5 && __GNUC_MINOR__ > 4))
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#endif
#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include <map>
#include <memory>
#include <string>

#include <boost/algorithm/string/erase.hpp>
#include <boost/type_index.hpp>
#include <spdlog/spdlog.h>

#ifdef JOYNR_ENABLE_STDOUT_LOGGING
#include <spdlog/sinks/stdout_sinks.h>
#endif // JOYNR_ENABLE_STDOUT_LOGGING
#ifdef JOYNR_ENABLE_DLT_LOGGING
#include "joynr/DltSink.h"
#endif // JOYNR_ENABLE_DLT_LOGGING

namespace joynr
{
enum class LogLevel { Trace, Debug, Info, Warn, Error, Fatal };
} // namespace joynr

#ifdef JOYNR_MAX_LOG_LEVEL_FATAL
#define JOYNR_LOG_LEVEL joynr::LogLevel::Fatal
#endif // JOYNR_MAX_LOG_LEVEL_FATAL

#ifdef JOYNR_MAX_LOG_LEVEL_ERROR
#define JOYNR_LOG_LEVEL joynr::LogLevel::Error
#endif // JOYNR_MAX_LOG_LEVEL_ERROR

#ifdef JOYNR_MAX_LOG_LEVEL_WARN
#define JOYNR_LOG_LEVEL joynr::LogLevel::Warn
#endif // JOYNR_MAX_LOG_LEVEL_WARN

#ifdef JOYNR_MAX_LOG_LEVEL_INFO
#define JOYNR_LOG_LEVEL joynr::LogLevel::Info
#endif // JOYNR_MAX_LOG_LEVEL_INFO

#ifdef JOYNR_MAX_LOG_LEVEL_DEBUG
#define JOYNR_LOG_LEVEL joynr::LogLevel::Debug
#endif // JOYNR_MAX_LOG_LEVEL_DEBUG

// default to Trace if no log level is set
#ifndef JOYNR_LOG_LEVEL
#define JOYNR_LOG_LEVEL joynr::LogLevel::Trace
#endif

#ifdef JOYNR_DEFAULT_RUNTIME_LOG_LEVEL_FATAL
#define JOYNR_DEFAULT_RUNTIME_LOG_LEVEL spdlog::level::critical
#endif // JOYNR_DEFAULT_RUNTIME_LOG_LEVEL_FATAL

#ifdef JOYNR_DEFAULT_RUNTIME_LOG_LEVEL_ERROR
#define JOYNR_DEFAULT_RUNTIME_LOG_LEVEL spdlog::level::err
#endif // JOYNR_DEFAULT_RUNTIME_LOG_LEVEL_ERROR

#ifdef JOYNR_DEFAULT_RUNTIME_LOG_LEVEL_WARN
#define JOYNR_DEFAULT_RUNTIME_LOG_LEVEL spdlog::level::warn
#endif // JOYNR_DEFAULT_RUNTIME_LOG_LEVEL_WARN

#ifdef JOYNR_DEFAULT_RUNTIME_LOG_LEVEL_INFO
#define JOYNR_DEFAULT_RUNTIME_LOG_LEVEL spdlog::level::info
#endif // JOYNR_DEFAULT_RUNTIME_LOG_LEVEL_INFO

#ifdef JOYNR_DEFAULT_RUNTIME_LOG_LEVEL_DEBUG
#define JOYNR_DEFAULT_RUNTIME_LOG_LEVEL spdlog::level::debug
#endif // JOYNR_DEFAULT_RUNTIME_LOG_LEVEL_DEBUG

#ifndef JOYNR_DEFAULT_RUNTIME_LOG_LEVEL
#define JOYNR_DEFAULT_RUNTIME_LOG_LEVEL spdlog::level::debug
#endif

#define JOYNR_CONDITIONAL_SPDLOG(level, method, logger, ...)                                       \
    do {                                                                                           \
        joynr::LogLevel logLevel = level;                                                          \
        if (logLevel == joynr::LogLevel::Debug && logger.getMapDebugToInfoLogsState()) {           \
            logLevel = joynr::LogLevel::Info;                                                      \
        }                                                                                          \
        joynr::LogLevel actualLogLevel = logger.getLogLevel();                                     \
        if (JOYNR_LOG_LEVEL <= logLevel && actualLogLevel <= logLevel) {                           \
            if (logLevel == joynr::LogLevel::Info) {                                               \
                logger.spdlog->info(__VA_ARGS__);                                                  \
            } else {                                                                               \
                logger.spdlog->method(__VA_ARGS__);                                                \
            }                                                                                      \
        }                                                                                          \
    } while (false)

#define JOYNR_LOG_TRACE(logger, ...)                                                               \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Trace, trace, logger, __VA_ARGS__)

#define JOYNR_LOG_DEBUG(logger, ...)                                                               \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Debug, debug, logger, __VA_ARGS__)

#define JOYNR_LOG_INFO(logger, ...)                                                                \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Info, info, logger, __VA_ARGS__)

#define JOYNR_LOG_WARN(logger, ...)                                                                \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Warn, warn, logger, __VA_ARGS__)

#define JOYNR_LOG_ERROR(logger, ...)                                                               \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Error, error, logger, __VA_ARGS__)

#define JOYNR_LOG_FATAL(logger, ...)                                                               \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Fatal, critical, logger, __VA_ARGS__)

#define ADD_LOGGER(T)                                                                              \
    static joynr::Logger& logger()                                                                 \
    {                                                                                              \
        static joynr::Logger instance(joynr::Logger::getPrefix<T>());                              \
        return instance;                                                                           \
    }

namespace joynr
{
struct LogLevelInitializer
{
    LogLevelInitializer()
    {
        const std::array<std::tuple<std::string, spdlog::level::level_enum, joynr::LogLevel>,
                         6> stringToSpdLogLevelToJoynrLogLevel{
                {std::make_tuple("TRACE", spdlog::level::trace, joynr::LogLevel::Trace),
                 std::make_tuple("DEBUG", spdlog::level::debug, joynr::LogLevel::Debug),
                 std::make_tuple("INFO", spdlog::level::info, joynr::LogLevel::Info),
                 std::make_tuple("WARN", spdlog::level::warn, joynr::LogLevel::Warn),
                 std::make_tuple("ERROR", spdlog::level::err, joynr::LogLevel::Error),
                 std::make_tuple("FATAL", spdlog::level::critical, joynr::LogLevel::Fatal)}};

        const char* logLevelEnv = std::getenv("JOYNR_LOG_LEVEL");

        if (logLevelEnv == nullptr) {
            spdlogLevel = JOYNR_DEFAULT_RUNTIME_LOG_LEVEL;
            spdlog::set_level(spdlogLevel);
            for (auto i : stringToSpdLogLevelToJoynrLogLevel) {
                if (std::get<1>(i) == JOYNR_DEFAULT_RUNTIME_LOG_LEVEL) {
                    level = std::get<2>(i);
                }
            }
            return;
        }

        const std::string runtimeLogLevelName(logLevelEnv);

        for (auto i : stringToSpdLogLevelToJoynrLogLevel) {
            if (std::get<0>(i) == runtimeLogLevelName) {
                spdlogLevel = std::get<1>(i);
                spdlog::set_level(spdlogLevel);
                level = std::get<2>(i);
                return;
            }
        }

        spdlog::set_level(JOYNR_DEFAULT_RUNTIME_LOG_LEVEL);
        for (auto i : stringToSpdLogLevelToJoynrLogLevel) {
            if (std::get<1>(i) == JOYNR_DEFAULT_RUNTIME_LOG_LEVEL) {
                level = std::get<2>(i);
            }
        }
    }

    joynr::LogLevel level;
    spdlog::level::level_enum spdlogLevel;
};

struct Logger
{
    explicit Logger(const std::string& prefix) : spdlog()
    {
        static LogLevelInitializer logLevelInitializer;
        level = logLevelInitializer.level;
        spdlogLevel = logLevelInitializer.spdlogLevel;
        std::vector<spdlog::sink_ptr> sinks;

#ifdef JOYNR_ENABLE_STDOUT_LOGGING
        auto sink1 = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        sink1->set_level(spdlog::level::trace);
        sinks.push_back(sink1);
#endif // JOYNR_ENABLE_STDOUT_LOGGING

#ifdef JOYNR_ENABLE_DLT_LOGGING
        auto sink2 = std::make_shared<joynr::DltSink>();
        sink2->set_level(spdlog::level::trace);
        sinks.push_back(sink2);
#endif // JOYNR_ENABLE_DLT_LOGGING

        spdlog = std::make_shared<spdlog::logger>(prefix, begin(sinks), end(sinks));
        spdlog->set_pattern(
                "%Y-%m-%d %H:%M:%S.%e [thread ID:%t] [%l] %n %v", spdlog::pattern_time_type::utc);
        spdlog->set_level(spdlogLevel);
    }

    template <typename Parent>
    static std::string getPrefix()
    {
        std::string prefix = boost::typeindex::type_id<Parent>().pretty_name();
        boost::algorithm::erase_all(prefix, "joynr::");
        return prefix;
    }

    inline joynr::LogLevel getLogLevel() const
    {
        return level;
    }

#ifdef JOYNR_DISABLE_DEBUG_TO_INFO_LOGGING
#define JOYNR_MAP_DEBUG_TO_INFO_LOG_INITIAL_STATE false
#else
#define JOYNR_MAP_DEBUG_TO_INFO_LOG_INITIAL_STATE true
#endif

    static bool& getMapDebugToInfoLogsState()
    {
        static bool mapDebugToInfoLogsState = JOYNR_MAP_DEBUG_TO_INFO_LOG_INITIAL_STATE;
        return mapDebugToInfoLogsState;
    }

    static void setMapDebugToInfoLogsState(bool enabled)
    {
        bool& mapDebugToInfoLogsState = getMapDebugToInfoLogsState();
        mapDebugToInfoLogsState = enabled;
    }

    std::shared_ptr<spdlog::logger> spdlog;
    joynr::LogLevel level;
    spdlog::level::level_enum spdlogLevel;
};

} // namespace joynr

#pragma GCC diagnostic pop

#endif // LOGGER_H
