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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.ArrayList;

import org.junit.Before;
import org.junit.Test;

import com.google.inject.Guice;
import com.google.inject.Key;
import com.google.inject.name.Names;

import io.joynr.common.ExpiryDate;
import io.joynr.exceptions.JoynrMessageExpiredException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;
import joynr.MutableMessage;

public class MessageRouterUtilTest {

    private MutableMessage joynrMessage;

    @Before
    public void setUp() {
        joynrMessage = new MutableMessage();
        joynrMessage.setSender("Mock");
        joynrMessage.setRecipient("Target");
        joynrMessage.setPayload("1234".getBytes());
    }

    private ImmutableMessage createMessage(boolean ttlAbsolute, long expiryDate) throws SecurityException,
                                                                                 EncodingException,
                                                                                 UnsuppportedVersionException {
        MutableMessage message = new MutableMessage();
        message.setSender("from");
        message.setRecipient("to");
        message.setTtlAbsolute(ttlAbsolute);
        message.setTtlMs(expiryDate);
        message.setPayload(new byte[]{ 'p', 'a', 'y', 'l', 'o', 'a', 'd' });
        return message.getImmutableMessage();
    }

    @Test
    public void isExpired_validMsg() throws SecurityException, EncodingException, UnsuppportedVersionException {
        ImmutableMessage msg = createMessage(true, System.currentTimeMillis() + 1200);
        assertFalse(MessageRouterUtil.isExpired(msg));
    }

    @Test
    public void isExpired_expiredMsg() throws SecurityException, EncodingException, UnsuppportedVersionException {
        ImmutableMessage msg = createMessage(true, System.currentTimeMillis() - 1);
        assertTrue(MessageRouterUtil.isExpired(msg));
    }

    @Test
    public void isExpired_negativeExpiryDate() throws SecurityException, EncodingException,
                                               UnsuppportedVersionException {
        ImmutableMessage msg = createMessage(true, -1);
        assertTrue(MessageRouterUtil.isExpired(msg));
    }

    @Test
    public void isExpired_relativeTtl() throws SecurityException, EncodingException, UnsuppportedVersionException {
        ImmutableMessage msg = createMessage(false, System.currentTimeMillis() + 1000);
        assertTrue(MessageRouterUtil.isExpired(msg));
    }

    @Test
    public void createDelayWithExponentialBackoff_noMaximum() {
        final long sendMsgRetryIntervalMs = 3000;
        Guice.createInjector(binder -> {
            binder.bind(Key.get(Long.class,
                                Names.named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS)))
                  .toInstance(sendMsgRetryIntervalMs);
            binder.bind(Key.get(Long.class,
                                Names.named(ConfigurableMessagingSettings.PROPERTY_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF_MS)))
                  .toInstance(ConfigurableMessagingSettings.DEFAULT_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF);
            binder.requestStaticInjection(MessageRouterUtil.class);
        });
        final int rounds = 100;
        ArrayList<Long> delays = new ArrayList<>(rounds);
        for (int i = 0; i < rounds; i++) {
            delays.add(MessageRouterUtil.createDelayWithExponentialBackoff(i));
        }
        assertFalse(delays.stream().anyMatch(d -> d < sendMsgRetryIntervalMs));
        assertTrue(delays.stream().anyMatch(d -> d > sendMsgRetryIntervalMs));
    }

    @Test
    public void createDelayWithExponentialBackoff_withMaximum() {
        final long sendMsgRetryIntervalMs = 3000;
        Guice.createInjector(binder -> {
            binder.bind(Key.get(Long.class,
                                Names.named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS)))
                  .toInstance(sendMsgRetryIntervalMs);
            binder.bind(Key.get(Long.class,
                                Names.named(ConfigurableMessagingSettings.PROPERTY_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF_MS)))
                  .toInstance(sendMsgRetryIntervalMs);
            binder.requestStaticInjection(MessageRouterUtil.class);
        });
        final int rounds = 100;
        ArrayList<Long> delays = new ArrayList<>(rounds);
        for (int i = 0; i < rounds; i++) {
            delays.add(MessageRouterUtil.createDelayWithExponentialBackoff(i));
        }
        assertFalse(delays.stream().anyMatch(d -> d != sendMsgRetryIntervalMs));
    }

    @Test
    public void checkExpiry_ttlRelativeNotExpired() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(1000).getValue());
        joynrMessage.setTtlAbsolute(false);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        try {
            MessageRouterUtil.checkExpiry(immutableMessage);
        } catch (JoynrMessageNotSentException e) {
            assertNotSame(e.getClass(), JoynrMessageExpiredException.class);
            return;
        }
        fail("Relative ttl not supported");
    }

    @Test
    public void checkExpiry_ttlRelativeExpired() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(1).getValue());
        joynrMessage.setTtlAbsolute(false);

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        Thread.sleep(5);

        try {
            MessageRouterUtil.checkExpiry(immutableMessage);
        } catch (JoynrMessageNotSentException e) {
            assertNotSame(e.getClass(), JoynrMessageExpiredException.class);
            return;
        }
        fail("Relative ttl not supported and message expired");
    }

    @Test
    public void checkExpiry_ttlAbsoluteExpired() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(1).getValue());
        joynrMessage.setTtlAbsolute(true);

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        Thread.sleep(5);

        try {
            MessageRouterUtil.checkExpiry(immutableMessage);
        } catch (JoynrMessageExpiredException e) {
            return;
        }
        fail("An expired message should throw");
    }

    @Test
    public void checkExpiry_ttlAbsoluteNotExpired() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(1000).getValue());
        joynrMessage.setTtlAbsolute(true);

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        try {
            MessageRouterUtil.checkExpiry(immutableMessage);
        } catch (JoynrMessageNotSentException e) {
            assertNotSame(e.getClass(), JoynrMessageExpiredException.class);
            fail("Absolute TTL should not be supported");
        }
    }
}
