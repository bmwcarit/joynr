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
#ifndef MULTICASTBROADCASTLISTENER_H
#define MULTICASTBROADCASTLISTENER_H

#include <memory>
#include <string>
#include <vector>

#include "joynr/AbstractBroadcastListener.h"
#include "joynr/JoynrExport.h"

namespace joynr
{

class PublicationManager;

/**
 * An attribute listener used for multicast broadcast subscriptions
 */
class JOYNR_EXPORT MulticastBroadcastListener : public AbstractBroadcastListener
{
public:
    /**
     * Create a multicast broadcast listener linked to a subscription
     * A multicast broadcast listener is being used also for regular
     * broadcasts.
     */
    MulticastBroadcastListener(const std::string& providerParticipantId,
                               std::weak_ptr<PublicationManager> publicationManager)
            : AbstractBroadcastListener(std::move(publicationManager)),
              _providerParticipantId(providerParticipantId)
    {
    }

    template <typename BroadcastFilter, typename... Ts>
    void selectiveBroadcastOccurred(const std::vector<std::shared_ptr<BroadcastFilter>>& filters,
                                    const Ts&... values);

    template <typename... Ts>
    void broadcastOccurred(const std::string& broadcastName,
                           const std::vector<std::string>& partitions,
                           const Ts&... values);

private:
    const std::string _providerParticipantId;
};

} // namespace joynr

#include "joynr/PublicationManager.h"

namespace joynr
{
template <typename BroadcastFilter, typename... Ts>
void MulticastBroadcastListener::selectiveBroadcastOccurred(
        const std::vector<std::shared_ptr<BroadcastFilter>>& filters,
        const Ts&... values)
{
    assert(false &&
           "MulticastBroadcastListner cannot handle selective broadcast. No subscriptionID.");
}

template <typename... Ts>
void MulticastBroadcastListener::broadcastOccurred(const std::string& broadcastName,
                                                   const std::vector<std::string>& partitions,
                                                   const Ts&... values)
{
    if (auto publicationManagerSharedPtr = _publicationManager.lock()) {
        publicationManagerSharedPtr->broadcastOccurred(
                broadcastName, _providerParticipantId, partitions, values...);
    }
}

} // namespace joynr

#endif // MULTICASTBROADCASTLISTENER_H
