/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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

#include "log4qt/consoleappender.h"
#include "log4qt/logger.h"
#include "log4qt/logmanager.h"
#include "log4qt/ttcclayout.h"
#include "log4qt/patternlayout.h"

#include <QtCore>

namespace joynr {

class Logging_log4qt : public joynr_logging::Logging {
public:
    Logging_log4qt();
    virtual void shutdown();

    virtual joynr_logging::Logger* getLogger(const QString contextId, const QString className);
    virtual void destroyLogger(const QString contextId, const QString className);
	
private:
    DISALLOW_COPY_AND_ASSIGN(Logging_log4qt);
    typedef QHash<QString, joynr_logging::Logger*> LoggerHash;
    LoggerHash loggers;

    QMutex loggerMutex;
};

class Logger_log4qt : public joynr_logging::Logger {
public:
    Logger_log4qt(const QString& prefix);

    virtual void log(joynr_logging::LogLevel logLevel, const char* message);
    virtual void log(joynr_logging::LogLevel logLevel, const QString& message);

private:
    DISALLOW_COPY_AND_ASSIGN(Logger_log4qt);
    static Log4Qt::Level joynrLogLevelToLog4QtLogLevel(joynr_logging::LogLevel logLevel);

    Log4Qt::Logger* logger;
};

Logging_log4qt::Logging_log4qt() :
    loggers(),
    loggerMutex()
{
    Log4Qt::LogManager::rootLogger();
//    Log4Qt::TTCCLayout *p_layout = new Log4Qt::TTCCLayout();
    Log4Qt::PatternLayout *p_layout = new Log4Qt::PatternLayout(QLatin1String("%d{ISO8601} [%t] [%-5p] %c: %m%n"));
    p_layout->setName(QLatin1String("Joynr Layout"));
    p_layout->activateOptions();
    // Create an appender
    Log4Qt::ConsoleAppender *p_appender = new Log4Qt::ConsoleAppender(p_layout, Log4Qt::ConsoleAppender::STDOUT_TARGET);
    p_appender->setName(QLatin1String("Joynr Appender"));
    p_appender->activateOptions();
    // Set appender on root logger
    Log4Qt::Logger::rootLogger()->addAppender(p_appender);
}

void Logging_log4qt::shutdown() {
}

joynr_logging::Logger* Logging_log4qt::getLogger(const QString contextId, const QString className) {
    QString prefix = contextId + " - " + className;
    if (!loggers.contains(prefix)) {
        QMutexLocker lock(&loggerMutex);
        if (!loggers.contains(prefix)) {
            loggers.insert(prefix, new Logger_log4qt(prefix));
        }
    }

    return loggers.value(prefix);
}

void Logging_log4qt::destroyLogger(const QString contextId, const QString className) {
    QString prefix = contextId + " - " + className;
    QMutexLocker lock(&loggerMutex);
    if (!loggers.contains(prefix)) {
        return;
    }
    delete loggers.value(prefix);
    loggers.remove(prefix);
}


Logger_log4qt::Logger_log4qt(const QString& prefix)
        : logger(Log4Qt::Logger::logger(prefix))
{
    logger->setLevel(Log4Qt::Level::TRACE_INT);
}

void Logger_log4qt::log(joynr_logging::LogLevel logLevel, const char* message) {
    logger->log(joynrLogLevelToLog4QtLogLevel(logLevel), message);
}

void Logger_log4qt::log(joynr_logging::LogLevel logLevel, const QString& message) {
    logger->log(joynrLogLevelToLog4QtLogLevel(logLevel), message);
}

Log4Qt::Level Logger_log4qt::joynrLogLevelToLog4QtLogLevel(joynr_logging::LogLevel logLevel) {
    switch (logLevel) {
    case joynr_logging::TRACE:
        return Log4Qt::Level::TRACE_INT;
    case joynr_logging::DEBUG:
        return Log4Qt::Level::DEBUG_INT;
    case joynr_logging::INFO:
        return Log4Qt::Level::INFO_INT;
    case joynr_logging::WARN:
        return Log4Qt::Level::WARN_INT;
    case joynr_logging::ERROR:
        return Log4Qt::Level::ERROR_INT;
    case joynr_logging::FATAL:
        return Log4Qt::Level::FATAL_INT;
    }

    return Log4Qt::Level::TRACE_INT;
}

joynr_logging::Logging* joynr_logging::Logging::getInstance() {
    static joynr_logging::Logging* instance = 0;
    static QMutex instanceMutex;
    if (instance == 0) {
        QMutexLocker lock(&instanceMutex);
        if (instance == 0) {
            instance = new Logging_log4qt();
        }
    }

    return instance;
}

} // namespace joynr
