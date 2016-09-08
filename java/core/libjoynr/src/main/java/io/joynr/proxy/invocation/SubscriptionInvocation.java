package io.joynr.proxy.invocation;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import io.joynr.proxy.Future;
import io.joynr.pubsub.SubscriptionQos;

public abstract class SubscriptionInvocation extends Invocation<String> {

    private String subscriptionId = "";
    private final String subscriptionName;
    private final SubscriptionQos qos;

    public SubscriptionInvocation(Future<String> future, String subscriptionName, SubscriptionQos qos) {
        this(future, subscriptionName, qos, null);
    }

    public SubscriptionInvocation(Future<String> future,
                                  String subscriptionName,
                                  SubscriptionQos qos,
                                  String subscriptionId) {
        super(future);
        this.subscriptionName = subscriptionName;
        this.subscriptionId = subscriptionId;
        this.qos = qos;
    }

    public boolean hasSubscriptionId() {
        return getSubscriptionId() != null && !getSubscriptionId().isEmpty();
    }

    public String getSubscriptionId() {
        return subscriptionId;
    }

    public void setSubscriptionId(String subscriptionId) {
        this.subscriptionId = subscriptionId;
    }

    public String getSubscriptionName() {
        return subscriptionName;
    }

    public SubscriptionQos getQos() {
        return qos;
    }

}