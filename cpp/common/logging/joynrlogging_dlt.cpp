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
#include "joynr/joynrlogging.h"

#include "dlt/dlt.h"

#include <QtCore>
#include <unordered_map>

namespace joynr {

class Logging_dlt : public joynr_logging::Logging {
public:
    Logging_dlt();
    virtual void shutdown();

    virtual joynr_logging::Logger* getLogger(const QString& contextId, const QString& className);
    virtual void destroyLogger(const QString& contextId, const QString& className);

private:
    typedef std::unordered_map<std::string, joynr_logging::Logger*> LoggerHash;
    typedef std::unordered_map<std::string, DltContext> ContextHash;

    LoggerHash loggers;
    ContextHash contextHash;

    std::mutex loggerMutex;
};

class Logger_dlt : public joynr_logging::Logger {
public:
    Logger_dlt(DltContext& dltContext, const QString& className);

    virtual void log(joynr_logging::LogLevel logLevel, const char* message);
    virtual void log(joynr_logging::LogLevel logLevel, const QString& message);

private:
    static DltLogLevelType joynrLogLevelToDltLogLevel(joynr_logging::LogLevel logLevel);

    DltContext& dltContext;
    QString prefix;
};

Logging_dlt::Logging_dlt() {
    DLT_REGISTER_APP("Joynr", "Joynr Framework");
    sleep(1); //Wait for logging to initialize
}

void Logging_dlt::shutdown() {
    DLT_UNREGISTER_APP();
}

joynr_logging::Logger* Logging_dlt::getLogger(const QString& contextId, const QString& className) {
    std::string loggerName = contextId.toStdString() + className.toStdString();
    if (loggers.find(loggerName) == loggers.end()) {
        std::lock_guard<std::mutex> lock(loggerMutex);
        if (loggers.find(loggerName) == loggers.end()) {
            if (contextHash.find(contextId.toStdString()) == contextHash.end()) {
                DLT_REGISTER_CONTEXT(contextHash[contextId], contextId.toAscii(), contextId.toAscii());
            }
            loggers.insert({loggerName, new Logger_dlt(contextHash[contextId], className)});
        }
    }

    return loggers.find(loggerName)->second;
}

void Logging_dlt::destroyLogger(const QString& contextId, const QString& className) {
    std::string loggerName = contextId.toStdString() + className.toStdString();
    std::lock_guard<std::mutex> lock(loggerMutex);
    if (loggers.find(loggerName) == loggers.end()) {
        return;
    }

    delete loggers.find(loggerName)->second;
    loggers.erase(loggerName);
}


Logger_dlt::Logger_dlt(DltContext& dltContext, const QString& className)
        : dltContext(dltContext),
        prefix("[" + className + "] ")
{
}

void Logger_dlt::log(joynr_logging::LogLevel logLevel, const char* message) {
    DLT_LOG(dltContext, joynrLogLevelToDltLogLevel(logLevel), DLT_STRING(prefix.toAscii()), DLT_STRING(message));
}

void Logger_dlt::log(joynr_logging::LogLevel logLevel, const QString& message) {
    DLT_LOG(dltContext, joynrLogLevelToDltLogLevel(logLevel), DLT_STRING(prefix.toAscii()), DLT_STRING(message.toAscii()));
}

DltLogLevelType Logger_dlt::joynrLogLevelToDltLogLevel(joynr_logging::LogLevel logLevel) {
    switch (logLevel) {
    case joynr_logging::TRACE:
        return DLT_LOG_VERBOSE;
    case joynr_logging::DEBUG:
        return DLT_LOG_DEBUG;
    case joynr_logging::INFO:
        return DLT_LOG_INFO;
    case joynr_logging::WARN:
        return DLT_LOG_WARN;
    case joynr_logging::ERROR:
        return DLT_LOG_ERROR;
    case joynr_logging::FATAL:
        return DLT_LOG_FATAL;
    }

    return DLT_LOG_VERBOSE;
}

joynr_logging::Logging* joynr_logging::Logging::getInstance() {
    static joynr_logging::Logging* instance = 0;
    static std::mutex instanceMutex;
    if (instance == 0) {
        std::unique_lock<std::mutex> lock(instanceMutex);
        if (instance == 0) {
            instance = new Logging_dlt();
        }
    }

    return instance;
}

} // namespace joynr
