package io.joynr.messaging.routing;

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

import java.util.concurrent.ScheduledExecutorService;

import javax.inject.Inject;
import javax.inject.Named;
import javax.inject.Singleton;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.accesscontrol.AccessController;
import io.joynr.accesscontrol.HasConsumerPermissionCallback;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import joynr.JoynrMessage;

public class CcMessageRouter extends AbstractMessageRouter {
    private static final Logger logger = LoggerFactory.getLogger(CcMessageRouter.class);
    private AccessController accessController;
    private boolean enableAccessControl;

    @Inject
    @Singleton
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 8 LINES
    public CcMessageRouter(RoutingTable routingTable,
                           @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                           @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                           MessagingStubFactory messagingStubFactory,
                           MessagingSkeletonFactory messagingSkeletonFactory,
                           AddressManager addressManager,
                           MulticastReceiverRegistry multicastReceiverRegistry,
                           AccessController accessController,
                           @Named(ClusterControllerRuntimeModule.PROPERTY_ACCESSCONTROL_ENABLE) boolean enableAccessControl) {
        super(routingTable,
              scheduler,
              sendMsgRetryIntervalMs,
              messagingStubFactory,
              messagingSkeletonFactory,
              addressManager,
              multicastReceiverRegistry);

        this.accessController = accessController;
        this.enableAccessControl = enableAccessControl;
    }

    @Override
    public void route(final JoynrMessage message) {
        if (enableAccessControl) {
            accessController.hasConsumerPermission(message, new HasConsumerPermissionCallback() {
                @Override
                public void hasConsumerPermission(boolean hasPermission) {
                    if (hasPermission) {
                        CcMessageRouter.super.route(message);
                    } else {
                        logger.warn("Dropping message {} from {} to {} because of insufficient access rights",
                                    message.getId(),
                                    message.getFrom(),
                                    message.getTo());
                    }
                }
            });
        } else {
            super.route(message);
        }
    }
}
