package io.joynr.dispatching.subscription;

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

import java.util.HashSet;
import java.util.Set;

import io.joynr.pubsub.publication.AttributeListener;
import io.joynr.pubsub.publication.BroadcastListener;
import io.joynr.pubsub.publication.MulticastListener;
import joynr.tests.testSubscriptionPublisherImpl;

public class SubscriptionTestsPublisher extends testSubscriptionPublisherImpl {
    Set<String> attributeSubscriptionArrived = new HashSet<>();

    public void waitForAttributeSubscription(String attributeName) {
        synchronized (this) {
            while (!attributeSubscriptionArrived.contains(attributeName)) {
                try {
                    this.wait();
                } catch (InterruptedException e) {
                }
            }
        }
    }

    public void waitForAttributeUnsubscription(String attributeName) {
        synchronized (this) {
            while (attributeSubscriptionArrived.contains(attributeName)) {
                try {
                    this.wait();
                } catch (InterruptedException e) {
                }
            }
        }
    }

    boolean broadcastSubscriptionArrived = false;

    boolean multicastSubscriptionArrived = false;

    public void waitForBroadcastSubscription() {
        synchronized (this) {
            while (!broadcastSubscriptionArrived) {
                try {
                    this.wait();
                } catch (InterruptedException e) {
                }
            }
        }
    }

    public void waitForMulticastSubscription() {
        synchronized (this) {
            while (!multicastSubscriptionArrived) {
                try {
                    this.wait();
                } catch (InterruptedException e) {
                }
            }
        }
    }

    @Override
    public void registerMulticastListener(MulticastListener multicastListener) {
        super.registerMulticastListener(multicastListener);
        if (!multicastSubscriptionArrived) {
            synchronized (this) {
                multicastSubscriptionArrived = true;
                this.notify();
            }
        }
    }

    @Override
    public void registerBroadcastListener(String broadcastName, BroadcastListener broadcastListener) {
        super.registerBroadcastListener(broadcastName, broadcastListener);
        if (!broadcastSubscriptionArrived) {
            synchronized (this) {
                broadcastSubscriptionArrived = true;
                this.notify();
            }
        }
    }

    @Override
    public void registerAttributeListener(String attributeName, AttributeListener attributeListener) {
        super.registerAttributeListener(attributeName, attributeListener);
        synchronized (this) {
            if (!attributeSubscriptionArrived.contains(attributeName)) {
                attributeSubscriptionArrived.add(attributeName);
                this.notify();
            }
        }
    }

    @Override
    public void unregisterAttributeListener(String attributeName, AttributeListener attributeListener) {
        super.unregisterAttributeListener(attributeName, attributeListener);
        synchronized (this) {
            if (attributeSubscriptionArrived.contains(attributeName)) {
                attributeSubscriptionArrived.remove(attributeName);
                this.notify();
            }
        }
    }
}
