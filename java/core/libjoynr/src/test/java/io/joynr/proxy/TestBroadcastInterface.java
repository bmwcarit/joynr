/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.proxy;

import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcBroadcast;
import io.joynr.exceptions.SubscriptionException;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import joynr.BroadcastFilterParameters;
import joynr.MulticastSubscriptionQos;
import joynr.OnChangeSubscriptionQos;

public interface TestBroadcastInterface {
    interface TestBroadcastListener extends BroadcastSubscriptionListener {
        void onReceive(final String testString);
    }

    class TestBroadcastAdapter implements TestBroadcastListener {
        @Override
        public void onReceive(final String testString) {
            // empty implementation
        }

        @Override
        public void onError(final SubscriptionException error) {
            // empty implementation
        }

        @Override
        public void onSubscribed(final String subscriptionId) {
            // empty implementation
        }
    }

    @JoynrRpcBroadcast(broadcastName = "testBroadcast")
    Future<String> subscribeToTestBroadcast(final TestBroadcastListener subscriptionListener,
                                            final OnChangeSubscriptionQos subscriptionQos,
                                            final BroadcastFilterParameters filterParameters);

    @JoynrMulticast(name = "testMulticast")
    Future<String> subscribeToTestMulticast(final TestBroadcastListener subscriptionListener,
                                            final MulticastSubscriptionQos subscriptionQos,
                                            final String... partitions);

    void unsubscribeFromTestBroadcast(final String subscriptionId);
}
