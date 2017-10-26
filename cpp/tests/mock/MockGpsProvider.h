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
#ifndef TESTS_MOCK_MOCKGPSPROVIDER_H
#define TESTS_MOCK_MOCKGPSPROVIDER_H

#include <string>

#include <gmock/gmock.h>

#include "joynr/Logger.h"
#include "joynr/vehicle/DefaultGpsProvider.h"

class MockGpsProvider : public joynr::vehicle::DefaultGpsProvider
{
public:
    MockGpsProvider() : joynr::vehicle::DefaultGpsProvider()
    {
    }

    ~MockGpsProvider()
    {
        JOYNR_LOG_DEBUG(logger(), "I am being destroyed");
    }

    MOCK_METHOD1(getLocation, void(joynr::types::Localisation::GpsLocation& result) );
    MOCK_METHOD1(setLocation, void(joynr::types::Localisation::GpsLocation gpsLocation));

    std::string getParticipantId() const
    {
        return "Fake_ParticipantId_vehicle/DefaultGpsProvider";
    }
private:
    ADD_LOGGER(MockGpsProvider)
};

#endif // TESTS_MOCK_MOCKGPSPROVIDER_H
