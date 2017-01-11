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
#include "joynr/Settings.h"
#include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
#include "joynr/Future.h"

namespace joynr
{

std::unique_ptr<JoynrRuntime> JoynrRuntime::createRuntime(
        const std::string& pathToLibjoynrSettings,
        const std::string& pathToMessagingSettings)
{

    return createRuntime(createSettings(pathToLibjoynrSettings, pathToMessagingSettings));
}

std::unique_ptr<JoynrRuntime> JoynrRuntime::createRuntime(std::unique_ptr<Settings> settings)
{
    Future<std::unique_ptr<JoynrRuntime>> runtimeFuture;

    auto onSuccessCallback = [&runtimeFuture](std::unique_ptr<JoynrRuntime> createdRuntime) {
        runtimeFuture.onSuccess(std::move(createdRuntime));
    };

    auto onErrorCallback = [&runtimeFuture](const exceptions::JoynrRuntimeException& exception) {
        runtimeFuture.onError(
                std::shared_ptr<joynr::exceptions::JoynrException>(exception.clone()));
    };

    createRuntimeAsync(
            std::move(settings), std::move(onSuccessCallback), std::move(onErrorCallback));

    std::unique_ptr<JoynrRuntime> runtime;
    runtimeFuture.get(runtime);

    return runtime;
}

void JoynrRuntime::createRuntimeAsync(
        const std::string& pathToLibjoynrSettings,
        std::function<void(std::unique_ptr<JoynrRuntime> createdRuntime)> runtimeCreatedCallback,
        std::function<void(const exceptions::JoynrRuntimeException& exception)>
                runtimeCreationErrorCallback,
        const std::string& pathToMessagingSettings)
{
    createRuntimeAsync(createSettings(pathToLibjoynrSettings, pathToMessagingSettings),
                       runtimeCreatedCallback,
                       runtimeCreationErrorCallback);
}

void JoynrRuntime::createRuntimeAsync(
        std::unique_ptr<Settings> settings,
        std::function<void(std::unique_ptr<JoynrRuntime> createdRuntime)> runtimeCreatedCallback,
        std::function<void(const exceptions::JoynrRuntimeException& exception)>
                runtimeCreationErrorCallback)
{
    std::ignore = runtimeCreationErrorCallback;

    struct ConfigurableDeleter
    {
        bool enabled = true;
        void operator()(JoynrRuntime* ptr)
        {
            if (enabled) {
                delete ptr;
            }
        }
    };

    std::shared_ptr<LibJoynrWebSocketRuntime> runtime(
            new LibJoynrWebSocketRuntime(std::move(settings)), ConfigurableDeleter());

    auto runtimeCreatedCallbackWrapper =
            [ runtime, runtimeCreatedCallback = std::move(runtimeCreatedCallback) ]()
    {
        // Workaround. Can't move an unique_ptr into the lambda
        std::unique_ptr<LibJoynrWebSocketRuntime> createdRuntime(runtime.get());
        std::get_deleter<ConfigurableDeleter>(runtime)->enabled = false;

        runtimeCreatedCallback(std::move(createdRuntime));
    };

    runtime->connect(std::move(runtimeCreatedCallbackWrapper));
}

std::unique_ptr<Settings> JoynrRuntime::createSettings(const std::string& pathToLibjoynrSettings,
                                                       const std::string& pathToMessagingSettings)
{
    auto settings = std::make_unique<Settings>(pathToLibjoynrSettings);

    Settings messagingSettings{pathToMessagingSettings};
    Settings::merge(messagingSettings, *settings, false);

    return settings;
}
} // namespace joynr
