/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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

import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.messaging.MessagingQos;
import joynr.types.DiscoveryEntryWithMetaInfo;
import org.junit.Before;
import org.mockito.Mock;

import java.lang.reflect.Method;
import java.util.HashSet;
import java.util.Set;
import java.util.UUID;

import static org.junit.Assert.fail;
import static org.mockito.MockitoAnnotations.openMocks;

public abstract class AbstractJoynrMessagingConnectorInvocationHandlerTest {

    protected static final String TO_PARTICIPANT_ID = UUID.randomUUID().toString();
    protected static final String FROM_PARTICIPANT_ID = UUID.randomUUID().toString();
    protected static final String STATELESS_ASYNC_PARTICIPANT_ID = UUID.randomUUID().toString();

    protected Method method;
    protected Object[] parameters;
    @SuppressWarnings("ClassEscapesDefinedScope")
    protected JoynrMessagingConnectorInvocationHandler handler;

    protected DiscoveryEntryWithMetaInfo toDiscoveryEntry;
    protected Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries;
    protected final Object proxy = new Object();
    protected final Future<String> future = new Future<>();
    @Mock
    protected RequestReplyManager requestReplyManager;
    @Mock
    protected ReplyCallerDirectory replyCallerDirectory;
    @Mock
    protected SubscriptionManager subscriptionManager;
    @Mock
    protected StatelessAsyncIdCalculator statelessAsyncIdCalculator;

    @Before
    public void setUp() {
        openMocks(this);

        toDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        toDiscoveryEntry.setParticipantId(TO_PARTICIPANT_ID);
        toDiscoveryEntries = new HashSet<>();
        toDiscoveryEntries.add(toDiscoveryEntry);

        handler = createHandler(toDiscoveryEntries);
    }

    private JoynrMessagingConnectorInvocationHandler createHandler(final Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries) {
        return new JoynrMessagingConnectorInvocationHandler(toDiscoveryEntries,
                                                            FROM_PARTICIPANT_ID,
                                                            new MessagingQos(),
                                                            requestReplyManager,
                                                            replyCallerDirectory,
                                                            subscriptionManager,
                                                            statelessAsyncIdCalculator,
                                                            STATELESS_ASYNC_PARTICIPANT_ID);
    }

    protected void addNewDiscoveryEntry() {
        final var newEntry = new DiscoveryEntryWithMetaInfo();
        newEntry.setParticipantId(UUID.randomUUID().toString());
        toDiscoveryEntries.add(newEntry);
    }

    @SuppressWarnings({ "rawtypes", "unchecked" })
    protected Method getMethod(final Class interfaceClass, final String methodName, final Class<?>... parameterTypes) {
        try {
            return interfaceClass.getDeclaredMethod(methodName, parameterTypes);
        } catch (final NoSuchMethodException e) {
            fail("Unexpected exception: " + e.getMessage());
            throw new RuntimeException(e);
        }
    }
}
