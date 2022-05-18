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

#include "tests/utils/Gtest.h"

#include <memory>

#include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
#include "joynr/Settings.h"
#include "joynr/MessagingSettings.h"
#include "joynr/LibjoynrSettings.h"

#include "tests/JoynrTest.h"

TEST(LibJoynrWebSocketRuntimeTest, destroyRuntimeWithoutInitCall)
{
    auto settings = std::make_unique<joynr::Settings>();
    joynr::MessagingSettings messagingSettings(*settings);
    joynr::LibjoynrSettings libJoynrSettings(*settings);

    // The init method won't be called until a websocket connection is established.
    // Therefore the destructor must be able to deinitialize the object correctly
    // when most members were not initialized yet.
    auto runtime = std::make_shared<joynr::LibJoynrWebSocketRuntime>(
            std::move(settings), failOnFatalRuntimeError);
    runtime->shutdown();
}
