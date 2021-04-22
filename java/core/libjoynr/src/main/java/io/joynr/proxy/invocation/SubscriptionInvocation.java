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
package io.joynr.proxy.invocation;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.SubscriptionListener;

public abstract class SubscriptionInvocation extends Invocation<String> {

    private String subscriptionId = "";
    private final String subscriptionName;
    private final SubscriptionQos qos;

    protected static boolean argsHasSubscriptionId(Object[] args) {
        return args[0] instanceof String;
    }

    protected static SubscriptionQos getQosParameter(Object[] args) {
        try {
            if (argsHasSubscriptionId(args)) {
                return (SubscriptionQos) args[2];
            }
            return (SubscriptionQos) args[1];
        } catch (ClassCastException e) {
            throw new JoynrIllegalStateException("subscribeTo must be passed a SubscriptionQos");
        }
    }

    @SuppressWarnings("unchecked")
    protected <T extends SubscriptionListener> T getSubscriptionListener(Object[] args) {
        try {
            if (argsHasSubscriptionId(args)) {
                return (T) args[1];
            }
            return (T) args[0];
        } catch (ClassCastException e) {
            throw new JoynrIllegalStateException("subscribeTo must be passed a SubscriptionListener");
        }
    }

    public SubscriptionInvocation(Future<String> future, String subscriptionName, SubscriptionQos qos) {
        super(future);
        this.subscriptionName = subscriptionName;
        this.subscriptionId = null;
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
