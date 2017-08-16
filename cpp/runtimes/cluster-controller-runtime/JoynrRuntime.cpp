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
#include "joynr/JoynrRuntime.h"
#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/Settings.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{
std::shared_ptr<JoynrRuntime> JoynrRuntime::createRuntime(
        const std::string& pathToLibjoynrSettings,
        const std::string& pathToMessagingSettings)
{
    auto settings = std::make_unique<Settings>(pathToLibjoynrSettings);
    Settings messagingSettings{pathToMessagingSettings};
    Settings::merge(messagingSettings, *settings, false);

    return createRuntime(std::move(settings));
}

std::shared_ptr<JoynrRuntime> JoynrRuntime::createRuntime(std::unique_ptr<Settings> settings)
{
    return JoynrClusterControllerRuntime::create(std::move(settings));
}

std::shared_ptr<JoynrRuntime> JoynrRuntime::createRuntimeAsync(
        const std::string& pathToLibjoynrSettings,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& exception)> onError,
        const std::string& pathToMessagingSettings)
{
    std::shared_ptr<JoynrRuntime> runtime;

    try {
        runtime = createRuntime(pathToLibjoynrSettings, pathToMessagingSettings);
        onSuccess();
    } catch (exceptions::JoynrRuntimeException& exception) {
        onError(exception);
    }
    return runtime;
}

std::shared_ptr<JoynrRuntime> JoynrRuntime::createRuntimeAsync(
        std::unique_ptr<Settings> settings,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& exception)> onError)
{
    std::shared_ptr<JoynrRuntime> runtime;
    try {
        runtime = createRuntime(std::move(settings));
        onSuccess();
    } catch (exceptions::JoynrRuntimeException& exception) {
        onError(exception);
    }
    return runtime;
}

} // namespace joynr
