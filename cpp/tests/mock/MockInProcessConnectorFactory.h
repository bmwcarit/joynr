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
#ifndef TESTS_MOCK_MOCKINPROCESSCONNECTORFACTORY_H
#define TESTS_MOCK_MOCKINPROCESSCONNECTORFACTORY_H

#include <memory>

#include <gmock/gmock.h>

#include "joynr/InProcessConnectorFactory.h"

#include "joynr/SubscriptionManager.h"
#include "joynr/PublicationManager.h"
#include "joynr/InProcessPublicationSender.h"

class MockInProcessConnectorFactory : public joynr::InProcessConnectorFactory {
public:

    MockInProcessConnectorFactory()
        : InProcessConnectorFactory(std::weak_ptr<joynr::SubscriptionManager>(),
                                    std::weak_ptr<joynr::PublicationManager>(),
                                    std::weak_ptr<joynr::InProcessPublicationSender>(),
                                    nullptr) {
    }

    MOCK_METHOD1(canBeCreated, bool(const std::shared_ptr<const joynr::system::RoutingTypes::Address> address));
};

#endif // TESTS_MOCK_MOCKINPROCESSCONNECTORFACTORY_H
