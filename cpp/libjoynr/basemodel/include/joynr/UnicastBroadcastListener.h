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
#ifndef UNICASTBROADCASTLISTENER_H
#define UNICASTBROADCASTLISTENER_H

#include <memory>
#include <string>
#include <vector>

#include "joynr/AbstractBroadcastListener.h"
#include "joynr/JoynrExport.h"

namespace joynr
{

class PublicationManager;

class JOYNR_EXPORT UnicastBroadcastListener : public AbstractBroadcastListener
{
public:
    /**
     * Create an unicast broadcast listener linked to a subscription
     * An unicast broadcast listener is being used for selectiveBroadcasts
     */
    UnicastBroadcastListener(const std::string& subscriptionId,
                             std::weak_ptr<PublicationManager> publicationManager)
            : AbstractBroadcastListener(publicationManager), _subscriptionId(subscriptionId)
    {
    }

    template <typename BroadcastFilter, typename... Ts>
    void selectiveBroadcastOccurred(const std::vector<std::shared_ptr<BroadcastFilter>>& filters,
                                    const Ts&... values);

    template <typename... Ts>
    void broadcastOccurred(const Ts&... values);

private:
    std::string _subscriptionId;
};

} // namespace joynr

#include "joynr/PublicationManager.h"

namespace joynr
{
template <typename BroadcastFilter, typename... Ts>
void UnicastBroadcastListener::selectiveBroadcastOccurred(
        const std::vector<std::shared_ptr<BroadcastFilter>>& filters,
        const Ts&... values)
{
    if (auto publicationManagerSharedPtr = _publicationManager.lock()) {
        publicationManagerSharedPtr->selectiveBroadcastOccurred(
                _subscriptionId, filters, values...);
    }
}

template <typename... Ts>
void UnicastBroadcastListener::broadcastOccurred(const Ts&... values)
{
    if (auto publicationManagerSharedPtr = _publicationManager.lock()) {
        publicationManagerSharedPtr->broadcastOccurred(_subscriptionId, values...);
    }
}

} // namespace joynr

#endif // UNICASTBROADCASTLISTENER_H
