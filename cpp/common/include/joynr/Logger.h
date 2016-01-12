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
#ifndef LOGGER_H
#define LOGGER_H

#include <memory>
#include <string>
#include <boost/type_index.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <spdlog/spdlog.h>

#define JOYNR_LOG_NULL(logger) logger

#ifdef JOYNR_MAX_LOG_LEVEL_FATAL
#define JOYNR_LOG_TRACE(logger) JOYNR_LOG_NULL(logger)
#define JOYNR_LOG_DEBUG(logger) JOYNR_LOG_NULL(logger)
#define JOYNR_LOG_INFO(logger) JOYNR_LOG_NULL(logger)
#define JOYNR_LOG_WARN(logger) JOYNR_LOG_NULL(logger)
#define JOYNR_LOG_ERROR(logger) JOYNR_LOG_NULL(logger)
#endif // JOYNR_MAX_LOG_LEVEL_FATAL

#ifdef JOYNR_MAX_LOG_LEVEL_ERROR
#define JOYNR_LOG_TRACE(logger) JOYNR_LOG_NULL(logger)
#define JOYNR_LOG_DEBUG(logger) JOYNR_LOG_NULL(logger)
#define JOYNR_LOG_INFO(logger) JOYNR_LOG_NULL(logger)
#define JOYNR_LOG_WARN(logger) JOYNR_LOG_NULL(logger)
#endif // JOYNR_MAX_LOG_LEVEL_ERROR

#ifdef JOYNR_MAX_LOG_LEVEL_WARN
#define JOYNR_LOG_TRACE(logger) JOYNR_LOG_NULL(logger)
#define JOYNR_LOG_DEBUG(logger) JOYNR_LOG_NULL(logger)
#define JOYNR_LOG_INFO(logger) JOYNR_LOG_NULL(logger)
#endif // JOYNR_MAX_LOG_LEVEL_WARN

#ifdef JOYNR_MAX_LOG_LEVEL_INFO
#define JOYNR_LOG_TRACE(logger) JOYNR_LOG_NULL(logger)
#define JOYNR_LOG_DEBUG(logger) JOYNR_LOG_NULL(logger)
#endif // JOYNR_MAX_LOG_LEVEL_INFO

#ifdef JOYNR_MAX_LOG_LEVEL_DEBUG
#define JOYNR_LOG_TRACE(logger) JOYNR_LOG_NULL(logger)
#endif // JOYNR_MAX_LOG_LEVEL_DEBUG

#ifndef JOYNR_LOG_TRACE
#define JOYNR_LOG_TRACE(logger) logger.spdlog->trace()
#endif // JOYNR_LOG_TRACE

#ifndef JOYNR_LOG_DEBUG
#define JOYNR_LOG_DEBUG(logger) logger.spdlog->debug()
#endif // JOYNR_LOG_DEBUG

#ifndef JOYNR_LOG_INFO
#define JOYNR_LOG_INFO(logger) logger.spdlog->info()
#endif // JOYNR_LOG_INFO

#ifndef JOYNR_LOG_WARN
#define JOYNR_LOG_WARN(logger) logger.spdlog->warn()
#endif // JOYNR_LOG_WARN

#ifndef JOYNR_LOG_ERROR
#define JOYNR_LOG_ERROR(logger) logger.spdlog->error()
#endif // JOYNR_LOG_ERROR

#ifndef JOYNR_LOG_FATAL
#define JOYNR_LOG_FATAL(logger) logger.spdlog->emerg()
#endif // JOYNR_LOG_FATAL

#define ADD_LOGGER(T) static joynr::Logger logger
// this macro allows to pass typenames containing commas (i.e. templates) as a single argument to
// another macro
#define SINGLE_MACRO_ARG(...) __VA_ARGS__
#define INIT_LOGGER(T) joynr::Logger T::logger(joynr::Logger::getPrefix<T>())

namespace joynr
{

struct Logger
{
    Logger(const std::string& prefix) : spdlog(spdlog::stdout_logger_mt(prefix))
    {
        spdlog->set_level(spdlog::level::trace);
        spdlog->set_pattern("%Y-%m-%d %H:%M:%S.%e [thread ID:%t] [%l] %n %v");
    }

    template <typename Parent>
    static std::string getPrefix()
    {
        std::string prefix = boost::typeindex::type_id<Parent>().pretty_name();
        boost::algorithm::erase_all(prefix, "joynr::");
        return prefix;
    }

    // this is used when log level is disabled
    template <typename T>
    Logger& operator<<(T&&)
    {
        return *this;
    }

    std::shared_ptr<spdlog::logger> spdlog;
};

} // namespace joynr
#endif // LOGGER_H
