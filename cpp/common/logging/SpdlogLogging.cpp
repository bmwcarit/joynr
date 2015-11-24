/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/PrivateCopyAssign.h"
#include "joynr/joynrlogging.h"

#include <memory>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/sink.h"

#include <QString>
#include <unordered_map>
#include <mutex>

namespace joynr
{
/**
 * @brief The SpdlogLogging class implements Logging interface and provides logging
 * over spdlog library.
 */
class SpdlogLogging : public joynr_logging::Logging
{
public:
    /**
     * @brief SpdlogLogging default constructor
     */
    SpdlogLogging();
    /**
     * @brief shutdown do all necessary cleanup
     */
    virtual void shutdown();
    /**
     * @brief getLogger provides access to Logger for given contextId and className
     * @param contextId
     * @param className
     * @return
     */
    virtual joynr_logging::Logger* getLogger(const QString contextId, const QString className);
    /**
     * @brief destroyLogger
     * @param contextId
     * @param className
     */
    virtual void destroyLogger(const QString contextId, const QString className);

private:
    DISALLOW_COPY_AND_ASSIGN(SpdlogLogging);
    typedef std::unordered_map<std::string, joynr_logging::Logger*> LoggerHash;
    LoggerHash loggers;

    std::mutex loggerMutex;
};

/**
 * @brief The SpdlogLogger class implements Logger interface and provides
 * logger from spdlog library
 */
class SpdlogLogger : public joynr_logging::Logger
{
public:
    /**
     * @brief SpdlogLogger constructs logger marked with given prefix
     * @param prefix unique logger id
     */
    SpdlogLogger(const QString& prefix);
    /**
     * @brief log
     * @param logLevel
     * @param message as char pointer
     */
    virtual void log(joynr_logging::LogLevel logLevel, const char* message);
    /**
     * @brief log
     * @param logLevel
     * @param message as const string ref
     */
    virtual void log(joynr_logging::LogLevel logLevel, const QString& message);

private:
    void configureLogger();
    std::shared_ptr<spdlog::logger> logger;
    DISALLOW_COPY_AND_ASSIGN(SpdlogLogger);
};

SpdlogLogging::SpdlogLogging() : loggers(), loggerMutex()
{
}

void SpdlogLogging::shutdown()
{
}

joynr_logging::Logger* SpdlogLogging::getLogger(const QString contextId, const QString className)
{
    std::string prefix = contextId.toStdString() + "-" + className.toStdString();
    if (loggers.find(prefix) == loggers.end()) {
        std::lock_guard<std::mutex> lock(loggerMutex);
        if (loggers.find(prefix) == loggers.end()) {
            loggers.insert({prefix, new SpdlogLogger(QString::fromStdString(prefix))});
        }
    }

    return loggers.find(prefix)->second;
}

void SpdlogLogging::destroyLogger(const QString contextId, const QString className)
{
    std::string prefix = contextId.toStdString() + " - " + className.toStdString();
    std::lock_guard<std::mutex> lock(loggerMutex);
    if (loggers.find(prefix) == loggers.end()) {
        return;
    }

    delete loggers.find(prefix)->second;
    loggers.erase(prefix);
}

SpdlogLogger::SpdlogLogger(const QString& prefix) : logger(nullptr)
{
    // create logger
    logger = spdlog::stdout_logger_mt(prefix.toStdString());

    // configure logger
    configureLogger();
}

void SpdlogLogger::log(joynr_logging::LogLevel logLevel, const char* message)
{
    switch (logLevel) {
    case joynr_logging::DEBUG:
        logger->debug(message);
        break;
    case joynr_logging::INFO:
        logger->info(message);
        break;
    case joynr_logging::WARN:
        logger->warn(message);
        break;
    case joynr_logging::ERROR:
        logger->error(message);
        break;
    case joynr_logging::FATAL:
        logger->critical(message);
        break;
    case joynr_logging::TRACE:
    default:
        logger->trace(message);
    }
}

void SpdlogLogger::log(joynr_logging::LogLevel logLevel, const QString& message)
{
    this->log(logLevel, message.toStdString().c_str());
}

void SpdlogLogger::configureLogger()
{
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [thread ID:%t] [%l] %n %v");
}

joynr_logging::Logging* joynr_logging::Logging::getInstance()
{
    static joynr_logging::Logging* instance = new SpdlogLogging();
    return instance;
}

} // namespace joynr
