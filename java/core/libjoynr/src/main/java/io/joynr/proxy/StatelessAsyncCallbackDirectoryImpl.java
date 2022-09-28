/*-
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.UsedBy;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.runtime.SystemServicesSettings;
import io.joynr.util.AnnotationUtil;
import joynr.system.RoutingTypes.Address;

@Singleton
public class StatelessAsyncCallbackDirectoryImpl implements StatelessAsyncCallbackDirectory {

    private static final Logger logger = LoggerFactory.getLogger(StatelessAsyncCallbackDirectoryImpl.class);

    private Map<String, StatelessAsyncCallback> directory = new ConcurrentHashMap<>();

    private ReplyCallerDirectory replyCallerDirectory;

    private final StatelessAsyncIdCalculator statelessAsyncIdCalculator;

    private MessageRouter messageRouter;
    private Address libjoynrMessagingAddress;

    @Inject
    public StatelessAsyncCallbackDirectoryImpl(ReplyCallerDirectory replyCallerDirectory,
                                               StatelessAsyncIdCalculator statelessAsyncIdCalculator,
                                               MessageRouter messageRouter,
                                               @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress) {
        this.replyCallerDirectory = replyCallerDirectory;
        this.statelessAsyncIdCalculator = statelessAsyncIdCalculator;
        this.messageRouter = messageRouter;
        this.libjoynrMessagingAddress = dispatcherAddress;
    }

    public void register(StatelessAsyncCallback statelessAsyncCallback) {
        String useCase = statelessAsyncCallback.getUseCase();
        StatelessAsyncCallback oldValue = directory.putIfAbsent(useCase, statelessAsyncCallback);
        if (oldValue != null) {
            String message = String.format("Multiple registration attempts of stateless callback for use case %s. %s already registered. %s will not be registered.",
                                           useCase,
                                           oldValue,
                                           statelessAsyncCallback);
            logger.error(message);
            throw new JoynrIllegalStateException(message);
        }
        String interfaceName = getInterfaceName(statelessAsyncCallback);
        String callbackId = statelessAsyncIdCalculator.calculateStatelessCallbackId(interfaceName,
                                                                                    statelessAsyncCallback);
        replyCallerDirectory.addReplyCaller(callbackId,
                                            new StatelessAsyncReplyCaller(callbackId, statelessAsyncCallback),
                                            ExpiryDate.fromAbsolute(Long.MAX_VALUE));
        // Trigger calculation of the participant ID, so that the hashed UUID is registered at startup
        String statelessAsyncParticipantId = statelessAsyncIdCalculator.calculateParticipantId(interfaceName,
                                                                                               statelessAsyncCallback);
        // isGloballyVisible is not applicable for stateless async callbacks
        final boolean isGloballyVisible = false;
        logger.info("Adding route for stateless callback {} / {} / {}",
                    statelessAsyncParticipantId,
                    libjoynrMessagingAddress,
                    isGloballyVisible);
        // It will throw in case of wrong joynr configuration
        messageRouter.addNextHop(statelessAsyncParticipantId, libjoynrMessagingAddress, isGloballyVisible);
    }

    public StatelessAsyncCallback get(String useCase) {
        return directory.get(useCase);
    }

    private String getInterfaceName(StatelessAsyncCallback statelessAsyncCallback) {
        UsedBy usedBy = AnnotationUtil.getAnnotation(statelessAsyncCallback.getClass(), UsedBy.class);
        if (usedBy == null) {
            throw new JoynrIllegalStateException("No @UsedBy annotation found on " + statelessAsyncCallback.getClass());
        }
        try {
            return (String) usedBy.value().getField("INTERFACE_NAME").get(null);
        } catch (IllegalAccessException | NoSuchFieldException e) {
            throw new JoynrIllegalStateException("Unable to get interface name from " + usedBy, e);
        }
    }

}
