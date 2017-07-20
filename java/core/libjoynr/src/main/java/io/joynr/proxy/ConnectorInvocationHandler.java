package io.joynr.proxy;

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

import java.lang.reflect.Method;

import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;
import joynr.exceptions.ApplicationException;

public interface ConnectorInvocationHandler {

    Object executeAsyncMethod(Method method, Object[] args, Future<?> future);

    Object executeSyncMethod(Method method, Object[] args) throws ApplicationException;

    void executeOneWayMethod(Method method, Object[] args);

    void executeSubscriptionMethod(BroadcastSubscribeInvocation broadcastSubscription);

    void executeSubscriptionMethod(UnsubscribeInvocation unsubscribeInvocation);

    void executeSubscriptionMethod(AttributeSubscribeInvocation attributeSubscription);

    void executeSubscriptionMethod(MulticastSubscribeInvocation multicastSubscription);

}
