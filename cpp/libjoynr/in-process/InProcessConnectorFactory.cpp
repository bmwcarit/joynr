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

#include "joynr/InProcessConnectorFactory.h"
#include "libjoynr/joynr-messaging/DummyPlatformSecurityManager.h"

namespace joynr
{

INIT_LOGGER(InProcessConnectorFactory);

InProcessConnectorFactory::InProcessConnectorFactory(
        std::weak_ptr<ISubscriptionManager> subscriptionManager,
        std::weak_ptr<PublicationManager> publicationManager,
        std::weak_ptr<InProcessPublicationSender> inProcessPublicationSender,
        std::shared_ptr<IRequestCallerDirectory> requestCallerDirectory)
        : subscriptionManager(subscriptionManager),
          publicationManager(std::move(publicationManager)),
          inProcessPublicationSender(inProcessPublicationSender),
          requestCallerDirectory(requestCallerDirectory),
          securityManager(std::make_shared<DummyPlatformSecurityManager>())
{
}

} // namespace joynr
