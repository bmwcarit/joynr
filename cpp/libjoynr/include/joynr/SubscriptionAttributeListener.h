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
#ifndef SUBSCRIPTIONATTRIBUTELISTENER_H
#define SUBSCRIPTIONATTRIBUTELISTENER_H

#include <string>

#include "joynr/JoynrExport.h"

namespace joynr
{

class PublicationManager;

/**
 * An attribute listener used for onChange subscriptions
 */
class JOYNR_EXPORT SubscriptionAttributeListener
{
public:
    /**
     * Create an attribute listener linked to a subscription
     */
    SubscriptionAttributeListener(const std::string& subscriptionId,
                                  std::weak_ptr<PublicationManager> publicationManager)
            : _subscriptionId(subscriptionId), _publicationManager(std::move(publicationManager))
    {
    }

    template <typename T>
    void attributeValueChanged(const T& value);

private:
    std::string _subscriptionId;
    std::weak_ptr<PublicationManager> _publicationManager;
};

} // namespace joynr

#include "joynr/PublicationManager.h"

namespace joynr
{

template <typename T>
void SubscriptionAttributeListener::attributeValueChanged(const T& value)
{
    if (auto publicationManagerSharedPtr = _publicationManager.lock()) {
        publicationManagerSharedPtr->attributeValueChanged(_subscriptionId, value);
    }
}

} // namespace joynr

#endif // SUBSCRIPTIONATTRIBUTELISTENER_H
