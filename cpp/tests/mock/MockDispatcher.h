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
#ifndef TESTS_MOCK_MOCKDISPATCHER_H
#define TESTS_MOCK_MOCKDISPATCHER_H

#include "tests/utils/Gmock.h"

#include <memory>
#include <string>

#include "joynr/IDispatcher.h"
#include "joynr/MessagingQos.h"

class MockDispatcher : public joynr::IDispatcher {
public:
    MOCK_METHOD3(addReplyCaller, void(const std::string& requestReplyId,
                                      std::shared_ptr<joynr::IReplyCaller> replyCaller,
                                      const joynr::MessagingQos& qosSettings));
    MOCK_METHOD2(addRequestCaller, void(const std::string& participantId, std::shared_ptr<joynr::RequestCaller> requestCaller));
    MOCK_METHOD1(removeRequestCaller, void(const std::string& participantId));
    MOCK_METHOD1(receive, void(std::shared_ptr<joynr::ImmutableMessage> message));
    MOCK_METHOD1(registerSubscriptionManager, void(std::shared_ptr<joynr::ISubscriptionManager> subscriptionManager));
    MOCK_METHOD1(registerPublicationManager,void(std::weak_ptr<joynr::PublicationManager> publicationManager));
    MOCK_METHOD0(shutdown, void ());
};

#endif // TESTS_MOCK_MOCKDISPATCHER_H
