package io.joynr.messaging.routing;

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

import java.util.concurrent.ScheduledExecutorService;

import javax.inject.Inject;
import javax.inject.Singleton;

import com.google.inject.name.Named;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.runtime.GlobalAddressProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.RoutingTypesUtil;

public class CcMessageRouter extends MessageRouterImpl {
    private String replyToAddress;

    @Inject
    @Singleton
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 8 LINES
    public CcMessageRouter(GlobalAddressProvider globalAddressProvider,
                           RoutingTable routingTable,
                           @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                           @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                           MessagingStubFactory messagingStubFactory,
                           MessagingSkeletonFactory messagingSkeletonFactory,
                           AddressManager addressManager,
                           MulticastReceiverRegistry multicastReceiverRegistry) {
        super(routingTable,
              scheduler,
              sendMsgRetryIntervalMs,
              messagingStubFactory,
              messagingSkeletonFactory,
              addressManager,
              multicastReceiverRegistry);
        this.replyToAddress = null;

        globalAddressProvider.registerGlobalAddressesReadyListener(new TransportReadyListener() {
            @Override
            public void transportReady(Address address) {
                String globalAddressString = RoutingTypesUtil.toAddressString(address);
                replyToAddress = globalAddressString;
            }
        });
    }

    @Override
    protected String getReplyToAddress() {
        return replyToAddress;
    }
}
