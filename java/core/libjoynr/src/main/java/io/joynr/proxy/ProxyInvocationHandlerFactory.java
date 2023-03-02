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
package io.joynr.proxy;

import java.util.Optional;
import java.util.Set;

import com.google.inject.assistedinject.Assisted;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.messaging.MessagingQos;
import io.joynr.runtime.ShutdownNotifier;

public interface ProxyInvocationHandlerFactory {
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public ProxyInvocationHandler create(@Assisted("domains") Set<String> domains,
                                         @Assisted("interfaceName") String interfaceName,
                                         @Assisted("proxyParticipantId") String proxyParticipantId,
                                         DiscoveryQos discoveryQos,
                                         MessagingQos messagingQos,
                                         ShutdownNotifier shutdownNotifier,
                                         Optional<StatelessAsyncCallback> statelessAsyncCallback,
                                         boolean separateReplyReceiver);

}
