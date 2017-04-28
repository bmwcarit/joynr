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

#include <thread>
#include "boost/thread/barrier.hpp"
#include "gtest/gtest.h"

#include "libjoynr/common/CallContextStorage.h"

using namespace ::testing;

TEST(CallContextStorageTest, testLocalThreadStorage) {
    boost::barrier barrier(2);

    // Each thread sets its own call context and waits until
    // the other thread did the same. Then both threads check
    // whether their call context is returned by the storage

    std::thread thread1([&barrier]()
    {
        joynr::CallContext expectedCallContext;
        expectedCallContext.setPrincipal("principalId_1");

        joynr::CallContextStorage::set(expectedCallContext);

        barrier.wait();

        EXPECT_EQ(expectedCallContext, joynr::CallContextStorage::get());
    });

    std::thread thread2([&barrier]()
    {
        joynr::CallContext expectedCallContext;
        expectedCallContext.setPrincipal("principalId_2");

        joynr::CallContextStorage::set(expectedCallContext);

        barrier.wait();

        EXPECT_EQ(expectedCallContext, joynr::CallContextStorage::get());
    });

    thread1.join();
    thread2.join();
}
