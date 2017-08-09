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

#include "joynr/Future.h"
#include "joynr/Settings.h"
#include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"

namespace joynr
{

std::unique_ptr<JoynrRuntime> JoynrRuntime::createRuntime(
        const std::string& pathToLibjoynrSettings,
        const std::string& pathToMessagingSettings,
        std::shared_ptr<IKeychain> keyChain)
{

    return createRuntime(
            createSettings(pathToLibjoynrSettings, pathToMessagingSettings), std::move(keyChain));
}

std::unique_ptr<JoynrRuntime> JoynrRuntime::createRuntime(std::unique_ptr<Settings> settings,
                                                          std::shared_ptr<IKeychain> keyChain)
{
    Future<void> runtimeFuture;

    auto onSuccessCallback = [&runtimeFuture]() { runtimeFuture.onSuccess(); };

    auto onErrorCallback = [&runtimeFuture](const exceptions::JoynrRuntimeException& exception) {
        runtimeFuture.onError(
                std::shared_ptr<joynr::exceptions::JoynrException>(exception.clone()));
    };

    auto runtime = createRuntimeAsync(std::move(settings),
                                      std::move(onSuccessCallback),
                                      std::move(onErrorCallback),
                                      std::move(keyChain));
    runtimeFuture.get();
    return runtime;
}

std::unique_ptr<JoynrRuntime> JoynrRuntime::createRuntimeAsync(
        const std::string& pathToLibjoynrSettings,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& exception)> onError,
        const std::string& pathToMessagingSettings,
        std::shared_ptr<IKeychain> keyChain)
{
    return createRuntimeAsync(createSettings(pathToLibjoynrSettings, pathToMessagingSettings),
                              std::move(onSuccess),
                              std::move(onError),
                              std::move(keyChain));
}

std::unique_ptr<JoynrRuntime> JoynrRuntime::createRuntimeAsync(
        std::unique_ptr<Settings> settings,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& exception)> onError,
        std::shared_ptr<IKeychain> keyChain)
{
    auto runtime =
            std::make_unique<LibJoynrWebSocketRuntime>(std::move(settings), std::move(keyChain));
    runtime->connect(std::move(onSuccess), std::move(onError));
    // this is necessary for gcc 4.9
    return std::move(runtime);
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
