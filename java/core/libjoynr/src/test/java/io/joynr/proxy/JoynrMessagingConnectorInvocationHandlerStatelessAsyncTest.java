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

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.MessagingQos;
import joynr.Request;
import joynr.types.DiscoveryEntryWithMetaInfo;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.junit.MockitoJUnitRunner;

import java.lang.reflect.Method;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

@RunWith(MockitoJUnitRunner.class)
public class JoynrMessagingConnectorInvocationHandlerStatelessAsyncTest
        extends AbstractJoynrMessagingConnectorInvocationHandlerTest {

    @Test
    public void testExecuteStatelessAsyncMethodShouldFailIfMethodHasNoCallbackParam() {
        method = getStatelessAsyncMethod("testMethodWithoutCallbackParam", String.class);
        parameters = new Object[]{ "test" };

        try {
            handler.executeStatelessAsyncMethod(method, parameters);
            fail("Exception excepted");
        } catch (final JoynrIllegalStateException exception) {
            assertTrue(exception.getMessage().contains("Stateless async method"));
            assertTrue(exception.getMessage().contains("MessageIdCallback"));
            verify(requestReplyManager, never()).sendRequest(anyString(),
                                                             any(DiscoveryEntryWithMetaInfo.class),
                                                             any(Request.class),
                                                             any(MessagingQos.class));
        }
    }

    @Test
    public void testExecuteStatelessAsyncMethodShouldFailIfDiscoveryEntrySetHasMoreThanOneItem() {
        method = getStatelessAsyncMethod("testMethodWithoutCallbackParam", String.class);
        parameters = new Object[]{ "test" };
        addNewDiscoveryEntry();
        try {
            handler.executeStatelessAsyncMethod(method, parameters);
            fail("Exception excepted");
        } catch (final JoynrIllegalStateException exception) {
            assertTrue(exception.getMessage().contains("multiple participants"));
            verify(requestReplyManager, never()).sendRequest(anyString(),
                                                             any(DiscoveryEntryWithMetaInfo.class),
                                                             any(Request.class),
                                                             any(MessagingQos.class));
        }
    }

    @Test
    public void testExecuteStatelessAsyncMethodShouldSucceed() {
        method = getStatelessAsyncMethod("testMethod", MessageIdCallback.class);
        parameters = new Object[]{ mock(MessageIdCallback.class) };
        handler.executeStatelessAsyncMethod(method, parameters);

        final ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);

        verify(requestReplyManager).sendRequest(eq(STATELESS_ASYNC_PARTICIPANT_ID),
                                                eq(toDiscoveryEntry),
                                                requestCaptor.capture(),
                                                any(MessagingQos.class));

        final Request request = requestCaptor.getValue();
        assertNotNull(request);
        assertEquals(method.getName(), request.getMethodName());
    }

    private Method getStatelessAsyncMethod(final String methodName, final Class<?>... parameterTypes) {
        return getMethod(TestStatelessAsyncInterface.class, methodName, parameterTypes);
    }
}
