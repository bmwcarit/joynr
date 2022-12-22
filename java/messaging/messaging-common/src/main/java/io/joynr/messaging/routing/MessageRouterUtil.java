/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.messaging.routing;

import java.text.MessageFormat;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrMessageExpiredException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import joynr.ImmutableMessage;

public class MessageRouterUtil {
    private static final Logger logger = LoggerFactory.getLogger(MessageRouterUtil.class);

    @Inject
    @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS)
    private static long sendMsgRetryIntervalMs;

    @Inject(optional = true)
    @Named(ConfigurableMessagingSettings.PROPERTY_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF_MS)
    private static long maxDelayMs = ConfigurableMessagingSettings.DEFAULT_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF;

    public static boolean isExpired(final ImmutableMessage message) {
        if (!message.isTtlAbsolute()) {
            // relative ttl is not supported
            return true;
        }
        return (message.getTtlMs() <= System.currentTimeMillis());
    }

    public static long createDelayWithExponentialBackoff(int retries) {
        long millis = sendMsgRetryIntervalMs + (long) ((2 ^ (retries)) * sendMsgRetryIntervalMs * Math.random());
        if (maxDelayMs >= sendMsgRetryIntervalMs && millis > maxDelayMs) {
            millis = maxDelayMs;
        }
        logger.trace("Created delay of {}ms in retry {}", millis, retries);
        return millis;
    }

    public static void checkExpiry(final ImmutableMessage message) throws JoynrMessageNotSentException,
                                                                   JoynrMessageExpiredException {
        if (!message.isTtlAbsolute()) {
            throw new JoynrMessageNotSentException("Relative ttl not supported");
        }
        final long currentTimeMillis = System.currentTimeMillis();
        if (message.getTtlMs() <= currentTimeMillis) {
            String errorMessage = MessageFormat.format("Received expired message: (now ={0}). Dropping the message {1}",
                                                       currentTimeMillis,
                                                       message.getTrackingInfo());
            logger.trace(errorMessage);
            throw new JoynrMessageExpiredException(errorMessage);
        }
    }
}
