package io.joynr.dispatching;

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

import java.util.Set;

import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessagingQos;
import joynr.MulticastPublication;
import joynr.SubscriptionPublication;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.types.DiscoveryEntryWithMetaInfo;

public interface Dispatcher extends MessageArrivedListener {
    public void sendSubscriptionRequest(String fromParticipantId,
                                        Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                                        SubscriptionRequest subscriptionRequest,
                                        MessagingQos qosSettings);

    public void sendSubscriptionStop(String fromParticipantId,
                                     Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                                     SubscriptionStop subscriptionStop,
                                     MessagingQos qosSettings);

    public void sendSubscriptionPublication(String fromParticipantId,
                                            Set<String> toParticipantId,
                                            SubscriptionPublication publication,
                                            MessagingQos qosSettings);

    void sendSubscriptionReply(String fromParticipantId,
                               String toParticipantId,
                               SubscriptionReply subscriptionReply,
                               MessagingQos qosSettings);

    void sendMulticast(String fromParticipantId, MulticastPublication multicastPublication, MessagingQos messagingQos);
}
