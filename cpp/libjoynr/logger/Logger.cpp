/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include "joynr/Logger.h"

#include <array>
#include <cstdlib>
#include <tuple>

joynr::LogLevelInitializer::LogLevelInitializer()
{
    const std::array<std::tuple<std::string, spdlog::level::level_enum, joynr::LogLevel>, 6>
            stringToSpdLogLevelToJoynrLogLevel{
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
