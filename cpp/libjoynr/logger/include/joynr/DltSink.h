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
#ifndef DLTSINK_H
#define DLTSINK_H

#include <memory>
#include <mutex>
#include <string>

#include <dlt/dlt.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/formatter.h>
#include <spdlog/sinks/sink.h>

namespace joynr
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunsafe-loop-optimizations"

class DltSink : public spdlog::sinks::sink
{
private:
    struct DltContextHolder {
        DltContext dltContext;

        DltContextHolder() : dltContext()
        {
            dlt_register_context(&dltContext, "JOYN", "joynr DLT context");
        }

        ~DltContextHolder()
        {
            dlt_unregister_context(&dltContext);
        }
    };

    static DltContextHolder& getInstance()
    {
        static DltContextHolder instance;
        return instance;
    }

    std::mutex _mutex;
    std::unique_ptr<spdlog::formatter> _formatter;

public:
    DltSink() : _mutex(), _formatter(spdlog::details::make_unique<spdlog::pattern_formatter>()){};

    ~DltSink() override = default;

protected:
    void flush() override
    {
    }

    void log(const spdlog::details::log_msg& msg) override
    {
        DltLogLevelType dltLogLevel = DltLogLevelType::DLT_LOG_DEFAULT;
        switch (msg.level) {
        case spdlog::level::trace:
            dltLogLevel = DLT_LOG_VERBOSE;
            break;
        case spdlog::level::debug:
            dltLogLevel = DLT_LOG_DEBUG;
            break;
        case spdlog::level::info:
            dltLogLevel = DLT_LOG_INFO;
            break;
        case spdlog::level::warn:
            dltLogLevel = DLT_LOG_WARN;
            break;
        case spdlog::level::err:
            dltLogLevel = DLT_LOG_ERROR;
            break;
        case spdlog::level::critical:
            dltLogLevel = DLT_LOG_FATAL;
            break;
        default:
            break;
        // The following case is never used.
        case spdlog::level::off:
            dltLogLevel = DLT_LOG_VERBOSE;
            break;
        }
        DltContext& dltContext = getInstance().dltContext;
        constexpr std::size_t maxLength = 1024;
        spdlog::memory_buf_t formatted;
        _formatter->format(msg, formatted);
        const std::size_t length = formatted.size();
        const std::string fullLog = std::string(formatted.data(), formatted.size());
        if (length < maxLength) {
            DLT_LOG_STRING(dltContext, dltLogLevel, fullLog.c_str());
        } else {
            DLT_LOG_STRING(dltContext, dltLogLevel, "----START OF SPLITTED LOG");
            for (std::size_t i = 0; i < length; i += maxLength) {
                DLT_LOG_STRING(dltContext, dltLogLevel, fullLog.substr(i, maxLength).c_str());
            }
            DLT_LOG_STRING(dltContext, dltLogLevel, "----END OF SPLITTED LOG");
        }
    }

    void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _formatter = std::move(sink_formatter);
    }

    void set_pattern(const std::string& pattern) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _formatter = std::unique_ptr<spdlog::formatter>(new spdlog::pattern_formatter(pattern));
    }
};

#pragma GCC diagnostic pop
} // namespace joynr
#endif // DLTSINK_H
