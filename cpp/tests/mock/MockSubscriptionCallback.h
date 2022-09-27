/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef TESTS_MOCK_MOCKSUBSCRIPTIONCALLBACK_H
#define TESTS_MOCK_MOCKSUBSCRIPTIONCALLBACK_H

#include "tests/utils/Gmock.h"

#include "joynr/ISubscriptionCallback.h"

class MockSubscriptionCallback : public joynr::ISubscriptionCallback
{
public:
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
    MOCK_METHOD1(executePublication, void(joynr::BasePublication& publication));
    MOCK_METHOD1(execute, void(const joynr::SubscriptionReply& subscriptionReply));

    void execute(joynr::BasePublication&& subscriptionPublication) override
    {
        executePublication(subscriptionPublication);
    }
};

#endif // TESTS_MOCK_MOCKSUBSCRIPTIONCALLBACK_H
