/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "JoynrClusterControllerRuntime.h"
#include "joynr/Settings.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{
std::unique_ptr<JoynrRuntime> JoynrRuntime::createRuntime(
        const std::string& pathToLibjoynrSettings,
        const std::string& pathToMessagingSettings)
{
    auto settings = std::make_unique<Settings>(pathToLibjoynrSettings);
    Settings messagingSettings{pathToMessagingSettings};
    Settings::merge(messagingSettings, *settings, false);

    return createRuntime(std::move(settings));
}

std::unique_ptr<JoynrRuntime> JoynrRuntime::createRuntime(std::unique_ptr<Settings> settings)
{
    return JoynrClusterControllerRuntime::create(std::move(settings));
}

void JoynrRuntime::createRuntimeAsync(
        const std::string& pathToLibjoynrSettings,
        std::function<void(std::unique_ptr<JoynrRuntime> createdRuntime)> runtimeCreatedCallback,
        std::function<void(const exceptions::JoynrRuntimeException& exception)>
                runtimeCreationErrorCallback,
        const std::string& pathToMessagingSettings)
{
    try {
        runtimeCreatedCallback(std::unique_ptr<JoynrRuntime>(
                createRuntime(pathToLibjoynrSettings, pathToMessagingSettings)));
    } catch (exceptions::JoynrRuntimeException& exception) {
        runtimeCreationErrorCallback(exception);
    }
}

void JoynrRuntime::createRuntimeAsync(
        std::unique_ptr<Settings> settings,
        std::function<void(std::unique_ptr<JoynrRuntime> createdRuntime)> runtimeCreatedCallback,
        std::function<void(const exceptions::JoynrRuntimeException& exception)>
                runtimeCreationErrorCallback)
{
    try {
        runtimeCreatedCallback(std::unique_ptr<JoynrRuntime>(createRuntime(std::move(settings))));
    } catch (exceptions::JoynrRuntimeException& exception) {
        runtimeCreationErrorCallback(exception);
    }
}

} // namespace joynr
