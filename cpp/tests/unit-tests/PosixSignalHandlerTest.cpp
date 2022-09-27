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

#include <chrono>
#include <csignal>
#include <memory>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "runtimes/cluster-controller-runtime/signal-handler/PosixSignalHandler.h"
#include "tests/mock/MockClusterControllerSignalHandler.h"

using namespace joynr;
using namespace ::testing;

class PosixSignalHandlerTest : public ::testing::Test
{

public:
    PosixSignalHandlerTest()
            : mockSignalHandler(std::make_shared<MockClusterControllerSignalHandler>())
    {
        PosixSignalHandler::setHandleAndRegisterForSignals(mockSignalHandler);
    };

    ~PosixSignalHandlerTest()
    {
        PosixSignalHandler::stopSignalHandling();
    }

    std::shared_ptr<MockClusterControllerSignalHandler> mockSignalHandler;
};

TEST_F(PosixSignalHandlerTest, shutdownClusterController)
{
    EXPECT_CALL(*mockSignalHandler, shutdownClusterController()).Times(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    raise(SIGTERM);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(PosixSignalHandlerTest, startExternalCommunication)
{
    EXPECT_CALL(*mockSignalHandler, startExternalCommunication()).Times(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    raise(SIGUSR1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(PosixSignalHandlerTest, stopExternalCommunication)
{
    EXPECT_CALL(*mockSignalHandler, stopExternalCommunication()).Times(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    raise(SIGUSR2);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
