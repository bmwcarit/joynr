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

#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/UdsServer.h"

#include "runtimes/libjoynr-runtime/uds/LibJoynrUdsRuntime.h"

#include "tests/mock/MockCallback.h"
#include "tests/mock/MockUdsServerCallbacks.h"

using namespace joynr;
using namespace testing;

const std::string settingsFilename{"test-resources/uds-libjoynr.settings"};
const std::chrono::seconds waitPeriodForClientServerCommunication{5};

/** The fixture provides a garbage collector for LibJoynrUdsRuntime and memory save callback
 * implementations. */
class LibJoynrUdsRuntimeTest : public ::testing::Test
{
    std::vector<std::shared_ptr<LibJoynrUdsRuntime>> _garbageCollector;

protected:
    class TestLibJoynrUdsRuntime : public LibJoynrUdsRuntime
    {
    public:
        TestLibJoynrUdsRuntime(std::function<void(const joynr::exceptions::JoynrRuntimeException&)>
                                       onFatalRuntimeError)
                : LibJoynrUdsRuntime(std::make_unique<Settings>(settingsFilename),
                                     std::move(onFatalRuntimeError))
        {
        }

        void connect(std::function<void()> onSuccess,
                     std::function<void(const exceptions::JoynrRuntimeException&)> onError)
        {
            LibJoynrUdsRuntime::connect(onSuccess, onError);
        }
    };

    std::shared_ptr<LibJoynrUdsRuntime> connectRuntime(LibJoynrUdsRuntimeTest* myTest)
    {
        /*
         * The LibJoynrRuntime violates the SBRM principals by storing shared pointers to itself.
         * Hence
         */
        auto runtime = std::make_shared<TestLibJoynrUdsRuntime>(std::bind(
                &LibJoynrUdsRuntimeTest::onFatalRuntimeError, myTest, std::placeholders::_1));
        runtime->connect([myTest]() { myTest->_runtimeCallbacks.onSuccess(); },
                         [myTest](const exceptions::JoynrRuntimeException& e) {
                             myTest->_runtimeCallbacks.onError(
                                     std::make_shared<exceptions::JoynrRuntimeException>(e));
                         });
        _garbageCollector.push_back(runtime);
        return runtime;
    }

    std::unique_ptr<UdsServer> createServer()
    {
        Settings settings(settingsFilename);
        UdsSettings udsSettings(settings);
        return std::make_unique<UdsServer>(udsSettings);
    }

    std::unique_ptr<UdsServer> createServer(MockUdsServerCallbacks& mock)
    {
        auto result = createServer();
        result->setConnectCallback([&mock](
                const system::RoutingTypes::UdsClientAddress& id,
                std::shared_ptr<IUdsSender> remoteClient) { mock.connected(id, remoteClient); });
        result->setDisconnectCallback([&mock](const system::RoutingTypes::UdsClientAddress& id) {
            mock.disconnected(id);
        });
        result->setReceiveCallback(
                [&mock](const system::RoutingTypes::UdsClientAddress& id,
                        smrf::ByteVector&& val) { mock.received(id, std::move(val)); });
        return result;
    }

    Semaphore _onFatalRuntimeErrorSemaphore;

    void shutdown(std::shared_ptr<LibJoynrUdsRuntime>&& runtime)
    {
        runtime->shutdown();
        _garbageCollector.erase(
                std::find(_garbageCollector.begin(), _garbageCollector.end(), runtime));
    }

public:
    MockCallbackWithJoynrException<void> _runtimeCallbacks;

    ~LibJoynrUdsRuntimeTest()
    {
        for (auto& runtime : _garbageCollector) {
            runtime->shutdown();
        }
        /*
         * Assure that runtime callbacks are deleted, before deleting their bindings.
         */
        _garbageCollector.clear();
    }

    void onFatalRuntimeError(const exceptions::JoynrRuntimeException&)
    {
        _onFatalRuntimeErrorSemaphore.notify();
    }
};

TEST_F(LibJoynrUdsRuntimeTest, createAndDelete)
{
    EXPECT_CALL(_runtimeCallbacks, onSuccess()).Times(0);
    EXPECT_CALL(_runtimeCallbacks, onError(_)).Times(0);
    connectRuntime(this);
}

TEST_F(LibJoynrUdsRuntimeTest, shutdownAfterConnect)
{
    Semaphore serverSemaphore;
    MockUdsServerCallbacks serverCallbacks;
    /*
     * onSuccess of runtime is currently not checked, since it waits for
     * routing-proxy replies, which are not simulated by this test setup.
     */
    EXPECT_CALL(serverCallbacks, connected(_, _))
            .WillOnce(InvokeWithoutArgs(&serverSemaphore, &Semaphore::notify));
    EXPECT_CALL(_runtimeCallbacks, onSuccess()).Times(0);
    auto runtime = connectRuntime(this);
    auto server = createServer(serverCallbacks);
    server->start();

    ASSERT_TRUE(serverSemaphore.waitFor(waitPeriodForClientServerCommunication))
            << "Connection not monitored by server";
    Mock::VerifyAndClearExpectations(&serverCallbacks);

    EXPECT_CALL(serverCallbacks, disconnected(_))
            .WillOnce(InvokeWithoutArgs(&serverSemaphore, &Semaphore::notify));
    shutdown(std::move(runtime));
    EXPECT_TRUE(serverSemaphore.waitFor(waitPeriodForClientServerCommunication))
            << "Disconnection not monitored by server";
}

TEST_F(LibJoynrUdsRuntimeTest, shutdownWhileWaitingForConnect)
{
    // The init method won't be called until a UDS connection is established.
    // Therefore the destructor must be able to deinitialize the object correctly
    // when most members were not initialized yet.
    EXPECT_CALL(_runtimeCallbacks, onSuccess()).Times(0);
    EXPECT_CALL(_runtimeCallbacks, onError(_)).Times(0);
    connectRuntime(this);
    std::this_thread::sleep_for(waitPeriodForClientServerCommunication);
}

TEST_F(LibJoynrUdsRuntimeTest, connectionLoss)
{
    Semaphore serverSemaphore;
    MockUdsServerCallbacks serverCallbacks;
    /*
     * onSuccess of runtime is currently not checked, since it waits for
     * routing-proxy replies, which are not simulated by this test setup.
     */
    EXPECT_CALL(serverCallbacks, connected(_, _))
            .WillOnce(InvokeWithoutArgs(&serverSemaphore, &Semaphore::notify));
    connectRuntime(this);
    auto server = createServer(serverCallbacks);
    server->start();

    ASSERT_TRUE(serverSemaphore.waitFor(waitPeriodForClientServerCommunication))
            << "Connection not monitored by server";
    Mock::VerifyAndClearExpectations(&serverCallbacks);

    server.reset();
    EXPECT_TRUE(_onFatalRuntimeErrorSemaphore.waitFor(waitPeriodForClientServerCommunication))
            << "Fatal runtime error not monitored by runtime user";
}
