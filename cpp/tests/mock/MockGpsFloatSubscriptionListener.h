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
#ifndef TESTS_MOCK_MOCKGPSFLOATSUBSCRIPTIONLISTENER_H
#define TESTS_MOCK_MOCKGPSFLOATSUBSCRIPTIONLISTENER_H

#include "tests/utils/Gmock.h"

#include "joynr/ISubscriptionListener.h"
#include "joynr/types/Localisation/GpsLocation.h"

class MockGpsFloatSubscriptionListener
        : public joynr::ISubscriptionListener<joynr::types::Localisation::GpsLocation, float> {
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(onReceive, void(const joynr::types::Localisation::GpsLocation& value, const float&));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

#endif // TESTS_MOCK_MOCKGPSFLOATSUBSCRIPTIONLISTENER_H
