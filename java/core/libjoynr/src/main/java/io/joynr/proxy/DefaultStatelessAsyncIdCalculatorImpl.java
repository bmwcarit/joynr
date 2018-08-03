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

import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;

import java.lang.reflect.Method;
import java.util.Random;

@Singleton
public class DefaultStatelessAsyncIdCalculatorImpl implements StatelessAsyncIdCalculator {

    private static final Logger logger = LoggerFactory.getLogger(DefaultStatelessAsyncIdCalculatorImpl.class);
    public static final String METHOD_SEPARATOR = ":#:";
    public static final String CHANNEL_SEPARATOR = ":>:";
    public static final String REQUEST_REPLY_ID_SEPARATOR = "#";

    private final String channelId;
    private final Random random = new Random();

    @Inject
    public DefaultStatelessAsyncIdCalculatorImpl(@Named(MessagingPropertyKeys.CHANNELID) String channelId) {
        this.channelId = channelId;
    }

    @Override
    public String calculateParticipantId(String interfaceName, StatelessAsyncCallback statelessAsyncCallback) {
        return channelId + CHANNEL_SEPARATOR + calculateStatelessCallbackId(interfaceName, statelessAsyncCallback);
    }

    @Override
    public String calculateStatelessCallbackId(String interfaceName, StatelessAsyncCallback statelessAsyncCallback) {
        return interfaceName + ":~:" + statelessAsyncCallback.getUseCaseName();
    }

    @Override
    public String calculateStatelessCallbackMethodId(Method method) {
        StatelessCallbackCorrelation callbackCorrelation = method.getAnnotation(StatelessCallbackCorrelation.class);
        if (callbackCorrelation == null) {
            logger.error("Method {} on {} is missing StatelessCallbackCorrelation. Unable to generate callback method ID.",
                         method,
                         method.getDeclaringClass().getName());
            throw new JoynrRuntimeException("No StatelessCallbackCorrelation found on method " + method);
        }
        return callbackCorrelation.value();
    }

    @Override
    public String calculateStatelessCallbackRequestReplyId(Method method) {
        String requestReplyId = String.valueOf(random.nextLong());
        String methodId = calculateStatelessCallbackMethodId(method);
        return requestReplyId + REQUEST_REPLY_ID_SEPARATOR + methodId;
    }

    @Override
    public String extractMethodIdFromRequestReplyId(String requestReplyId) {
        if (requestReplyId == null || requestReplyId.trim().isEmpty()
                || !requestReplyId.contains(REQUEST_REPLY_ID_SEPARATOR)) {
            throw new JoynrIllegalStateException("Unable to extract method ID from invalid request/reply ID: "
                    + requestReplyId);
        }
        int index = requestReplyId.indexOf(REQUEST_REPLY_ID_SEPARATOR);
        return requestReplyId.substring(index + REQUEST_REPLY_ID_SEPARATOR.length());
    }

    @Override
    public String withoutMethod(String statelessCallback) {
        if (statelessCallback == null || statelessCallback.trim().isEmpty()
                || !statelessCallback.contains(METHOD_SEPARATOR)) {
            throw new JoynrRuntimeException("Stateless callback ID " + statelessCallback
                    + " invalid. Needs to be non-null and contain method ID after " + METHOD_SEPARATOR);
        }
        int separatorIndex = statelessCallback.indexOf(METHOD_SEPARATOR);
        return statelessCallback.substring(0, separatorIndex);
    }

    @Override
    public String fromParticpantAndMethod(String statelessParticipantId, String methodId) {
        if (statelessParticipantId == null || statelessParticipantId.trim().isEmpty()
                || !statelessParticipantId.contains(CHANNEL_SEPARATOR)) {
            throw new JoynrRuntimeException("Invalid stateless callback participant ID: " + statelessParticipantId
                    + ". Unable to extract callback ID.");
        }
        int separatorIndex = statelessParticipantId.indexOf(CHANNEL_SEPARATOR);
        String statelessCallbackId = statelessParticipantId.substring(separatorIndex + CHANNEL_SEPARATOR.length());
        return statelessCallbackId + METHOD_SEPARATOR + methodId;
    }
}
