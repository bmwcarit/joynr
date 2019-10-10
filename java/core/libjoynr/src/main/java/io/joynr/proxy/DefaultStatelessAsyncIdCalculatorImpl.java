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

import java.io.UnsupportedEncodingException;
import java.lang.reflect.Method;
import java.util.Map;
import java.util.Optional;
import java.util.Random;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;

@Singleton
public class DefaultStatelessAsyncIdCalculatorImpl implements StatelessAsyncIdCalculator {

    private static final Logger logger = LoggerFactory.getLogger(DefaultStatelessAsyncIdCalculatorImpl.class);

    private final String channelId;
    private final Random random = new Random();
    private final Map<String, String> participantIdMap = new ConcurrentHashMap<>();

    @Inject
    public DefaultStatelessAsyncIdCalculatorImpl(@Named(MessagingPropertyKeys.CHANNELID)
                                                 final String channelId) {
        this.channelId = channelId;
    }

    @Override
    public String calculateParticipantId(final String interfaceName, final StatelessAsyncCallback statelessAsyncCallback) {
        final String statelessCallbackId = calculateStatelessCallbackId(interfaceName, statelessAsyncCallback);
        final String fullParticipantId = channelId + CHANNEL_SEPARATOR + statelessCallbackId;
        try {
            final String uuid = UUID.nameUUIDFromBytes(fullParticipantId.getBytes("UTF-8")).toString();
            participantIdMap.putIfAbsent(uuid, statelessCallbackId);
            return uuid;
        } catch (final UnsupportedEncodingException e) {
            throw new JoynrRuntimeException("Platform does not support UTF-8", e);
        }
    }

    @Override
    public String calculateStatelessCallbackId(final String interfaceName, final StatelessAsyncCallback statelessAsyncCallback) {
        return interfaceName + USE_CASE_SEPARATOR + statelessAsyncCallback.getUseCase();
    }

    @Override
    public String calculateStatelessCallbackMethodId(final Method method) {
        final StatelessCallbackCorrelation callbackCorrelation = method.getAnnotation(StatelessCallbackCorrelation.class);
        if (callbackCorrelation == null) {
            logger.error("Method {} on {} is missing StatelessCallbackCorrelation. Unable to generate callback method ID.",
                         method,
                         method.getDeclaringClass().getName());
            throw new JoynrRuntimeException("No StatelessCallbackCorrelation found on method " + method);
        }
        return callbackCorrelation.value();
    }

    @Override
    public String calculateStatelessCallbackRequestReplyId(final Method method) {
        final String requestReplyId = String.valueOf(random.nextLong());
        final String methodId = calculateStatelessCallbackMethodId(method);
        return requestReplyId + REQUEST_REPLY_ID_SEPARATOR + methodId;
    }

    @Override
    public String extractMethodIdFromRequestReplyId(final String requestReplyId) {
        if (requestReplyId == null || requestReplyId.trim().isEmpty()
                || !requestReplyId.contains(REQUEST_REPLY_ID_SEPARATOR)) {
            throw new JoynrIllegalStateException("Unable to extract method ID from invalid request/reply ID: "
                    + requestReplyId);
        }
        final int index = requestReplyId.indexOf(REQUEST_REPLY_ID_SEPARATOR);
        return requestReplyId.substring(index + REQUEST_REPLY_ID_SEPARATOR.length());
    }

    @Override
    public String fromParticipantUuid(final String statelessParticipantIdUuid) {
        String statelessCallbackId = null;
        try {
            statelessCallbackId = Optional.ofNullable(participantIdMap.get(statelessParticipantIdUuid))
                                          .orElseThrow(() -> new JoynrIllegalStateException("Unknown stateless participant ID UUID: "
                                                  + statelessParticipantIdUuid));
        } catch (final Throwable throwable) {
            throwable.printStackTrace();
            // there is an issue for OpenJDK8 where type inference of generic exceptions is not
            // working correctly, so we maintain previous behavior by throwing JoynrIllegalStateException
            if (throwable instanceof JoynrIllegalStateException) {
                throw (JoynrIllegalStateException) throwable;
            }
        }
        return statelessCallbackId;
    }

}
