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

#include <boost/config.hpp>

#include "utils/Gmock.h"
#include "utils/Gtest.h"

#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

#ifdef BOOST_HAS_PTHREAD
#include <pthread.h>
void setThreadName(const char* name)
{
    pthread_setname_np(pthread_self(), name);
}
#else
void setThreadName(...)
{
}
#endif // BOOST_HAS_PTHREAD

int main(int argc, char* argv[])
{
// Register app at the dlt-daemon for logging
#ifdef JOYNR_ENABLE_DLT_LOGGING
    DLT_REGISTER_APP("JOYT", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    setThreadName("main");

    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
