/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#ifndef SUBSCRIPTIONBROADCASTLISTENER_H
#define SUBSCRIPTIONBROADCASTLISTENER_H

#include <string>
#include <memory>
#include <vector>

#include "joynr/Variant.h"
#include "joynr/JoynrExport.h"

namespace joynr
{

class PublicationManager;

/**
 * An attribute listener used for broadcast subscriptions
 */
class JOYNR_EXPORT SubscriptionBroadcastListener
{
public:
    /**
     * Create an broadcast listener linked to a subscription
     */
    SubscriptionBroadcastListener(const std::string& subscriptionId,
                                  PublicationManager& publicationManager)
            : subscriptionId(subscriptionId), publicationManager(publicationManager)
    {
    }

    template <typename BroadcastFilter, typename... Ts>
    void selectiveBroadcastOccurred(const std::vector<std::shared_ptr<BroadcastFilter>>& filters,
                                    const Ts&... values);

    template <typename... Ts>
    void broadcastOccurred(const Ts&... values);

private:
    std::string subscriptionId;
    PublicationManager& publicationManager;
};

} // namespace joynr

#include "joynr/PublicationManager.h"

namespace joynr
{
template <typename BroadcastFilter, typename... Ts>
void SubscriptionBroadcastListener::selectiveBroadcastOccurred(
        const std::vector<std::shared_ptr<BroadcastFilter>>& filters,
        const Ts&... values)
{
    publicationManager.selectiveBroadcastOccurred(subscriptionId, filters, values...);
}

template <typename... Ts>
void SubscriptionBroadcastListener::broadcastOccurred(const Ts&... values)
{
    publicationManager.broadcastOccurred(subscriptionId, values...);
}

} // namespace joynr

#endif // SUBSCRIPTIONBROADCASTLISTENER_H
