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
#ifndef TESTS_MOCK_MOCKTRANSPORTMESSAGESENDER_H
#define TESTS_MOCK_MOCKTRANSPORTMESSAGESENDER_H

#include "tests/utils/Gmock.h"

#include "joynr/ITransportMessageSender.h"

class MockTransportMessageSender : public joynr::ITransportMessageSender
{
public:
    MOCK_METHOD3(sendMessage,
                 void(const joynr::system::RoutingTypes::Address&,
                      std::shared_ptr<joynr::ImmutableMessage>,
                      const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&));
};

#endif // TESTS_MOCK_MOCKTRANSPORTMESSAGESENDER_H
